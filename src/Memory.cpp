#include <algorithm>
#include <iostream>
#include <sstream>
#include <rlog/rlog.h>
#include <errno.h>
#include <cstring>

#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Memory.hpp"
#include "LinearMap.hpp"

Memory::Memory(const struct stat *st, const char *name) :
	Parent (st, name),
	m_FileSize (0),
	m_FileSizeSet (false)
{
}

Memory::~Memory()
{
	assert (m_LinearMap.empty());
}

void Memory::Print(ostream &os) const
{
	os << "m_FileSize: " << m_FileSize << ", m_FileSizeSet: " << m_FileSizeSet << endl;
	os << "m_LinearMap: " << endl << m_LinearMap << endl << endl;
}

ostream &operator<<(ostream &os, const Memory &rMemory)
{
	rMemory.Print(os);
	return os;
}

/**
 * Merge memory map with content in the file on the disk.
 */
int Memory::merge(const char *name)
{
	int r = 1;
	
	if (m_LinearMap.empty())
		return 0;

	while (r == 1)
	{
		r = write(true);
		if (r == -1)
		{
			int tmp = errno;
			rError("Memory::Merge Cannot write"
			       "data to file '%s' '%s', error: %s",
				m_FileName.c_str(), name, strerror(errno));
			errno = tmp;

			// Error happend, don't try to continue. But
			// don't forget to release allocated memory...
			//
			truncate(name, 0);
			
			return -1;
		}
	}

	return 0;
}

int Memory::open(const char *name, int flags)
{
	if (m_FileSizeSet == false)
	{
		struct stat st;

		int r = Parent::getattr(name, &st);
		if (r != 0)
		{
			rDebug("%s Parent::getattr(%s, ...) failed (%s)",
				__PRETTY_FUNCTION__, name, strerror(errno));
			return -1;
		}
		m_FileSize = st.st_size;
		m_FileSizeSet = true;
	}
	return Parent::open(name, flags);
}

int Memory::release(const char *name)
{
	int rm;
	int rr;

	// Store all cached data in ram to file.
	//
	rm = merge(name);

	// Release lower file.
	//
	rr = Parent::release(name);

	m_FileSize = 0;
	m_FileSizeSet = false;

	// Return any error...
	//
	return rm ? rm : rr;
}

int Memory::unlink(const char *name)
{
	rDebug("Memory::unlink(%s)", m_FileName.c_str());

	int r = Parent::unlink(name);
	if (r == 0)
	{
		// Lower file deleted, release all memory
		// buffers allocated so far and set m_FileSize
		// to default (-1) because the object remains in
		// memory and when a new node with the same inode is
		// created it will reuse this object again...
		// 
		m_LinearMap.truncate(0);
		m_FileSize = 0;
		m_FileSizeSet = false;
	}
	return r;
}

int Memory::truncate(const char *name, off_t size)
{
	rDebug("Memory::truncate(%s) size: 0x%llx", m_FileName.c_str(),
			(long long int) size);

	int r = Parent::truncate(name, size);
	if (r == 0)
	{
		m_LinearMap.truncate(size);
		m_FileSize = size;
		m_FileSizeSet = true;
	}
	return r;
}

int Memory::getattr(const char *name, struct stat *st)
{
	int r = Parent::getattr(name, st);

	rDebug("Memory::getattr m_FileSize: 0x%llx, m_FileSizeSet: %d", m_FileSize, m_FileSizeSet);

	if (m_FileSizeSet == true)
	{
		st->st_size = m_FileSize;
	}

	return r;
}

int Memory::write(bool force)
{
	char	*buf;
	size_t	 size;
	ssize_t	 len;
	off_t	 offset;

	if (m_LinearMap.erase(&offset, &buf, &size, force) == true)
	{
		len = Parent::write(buf, size, offset);
		delete[] buf;
		return (len == -1) ? -1 : 1;
	}

	return 0;
}

ssize_t Memory::write(const char *buf, size_t size, off_t offset)
{
	rDebug("Memory::write(%s) | m_FileSize: 0x%llx, offset: 0x%llx, size: 0x%x",
			m_FileName.c_str(), (long long int) m_FileSize,
			(long long int) offset, (unsigned int) size);

	// Store buffer to memory in LinearMap.
	//
	if (m_LinearMap.put(buf, size, offset) == -1)
		return -1;

	assert(m_FileSizeSet == true);
	m_FileSize = max(m_FileSize, offset + (off_t) size);

	// Try to write a block to disk if appropriate.
	// 
	int r = write(false);
	if (r == -1)
		return r;

	return size;
}

ssize_t Memory::read(char *buf, size_t size, off_t offset)
{
	size_t	 osize = size;
	ssize_t	 len = size;

	rDebug("Memory::read(%s) | m_FileSize: 0x%llx, offset: 0x%llx, size: 0x%x",
			m_FileName.c_str(), (long long int) m_FileSize,
			(long long int) offset, (unsigned int) size);

	assert(m_FileSizeSet == true);

	if (offset > m_FileSize)
	{
		rDebug("Memory::read ret: 0x%lx", 0);
		return 0;
	}
	while (len > 0)
	{
		char	*block;
		size_t	 block_size;

		off_t block_offset = m_LinearMap.get(offset, &block, &block_size);

		rDebug("block_offset: 0x%llx, offset: 0x%llx, block: %p, block_size: 0x%llx",
			block_offset, offset, block, block_size);

		if (block_offset == -1)
		{
			// Block not found, there isn't any block higher too.
			// Try to read the data form the Parent. Don't forget
			// to fill up with zeros to m_FileSize if Parent read
			// returns less.

			int r = Parent::read(buf, len, offset);
			rDebug("Memory::read Parent::read(1) returned 0x%lx", r);
			if (r == -1)
				return -1;

			offset += r;
			buf += r;
			len -= r;
			
			size_t tmp = min(m_FileSize - offset, len);
			memset(buf, 0, tmp);

			len -= tmp;
			break;
		}

		size_t cs;

		if (block_offset == offset)
		{
			cs = min(block_size, (size_t) len);
			memcpy(buf, block, cs);
		}
		else
		{
			// m_FileSize, offset, size, block_offset
			//
			cs = min(block_offset - offset, (off_t) len);
			cs = Parent::read(buf, cs, offset);
			rDebug("Memory::read Parent::read(2) returned 0x%lx", cs);
			if (cs == -1)
				return -1;
			if (cs == 0)
			{
				cs = min(block_offset - offset, (off_t) len);
				memset(buf, 0, cs);
			}
		}
		offset += cs;
		buf += cs;
		len -= cs;
	}
	rDebug("Memory::read ret: 0x%lx", osize - len);
	return osize - len;
}

