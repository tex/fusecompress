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

#include "FileUtils.hpp"
#include "Memory.hpp"
#include "LinearMap.hpp"

Memory::Memory(const struct stat *st, const char *name) :
	Parent (st, name),
	m_FileSize (0),
	m_FileSizeSet (false),
	m_TimeSet (false)
{
}

Memory::~Memory()
{
	assert (m_LinearMap.empty());
}

ostream &operator<<(ostream &os, const Memory &rM)
{
	os << "--- m_FileSize: " << rM.m_FileSize << ", m_FileSizeSet: " << rM.m_FileSizeSet << std::endl;
	os << "m_LinearMap: " << std::endl << rM.m_LinearMap << std::endl;
	os << "---" << std::endl;
	return os;
}

/**
 * Merge memory map with content in the file on the disk.
 */
int Memory::merge(const char *name)
{
	assert(m_name == name);

	if (m_LinearMap.empty() == false)
	{
		int r;

		// There is (are) some block(s) to be written.

		r = write(true);
		if (r == -1)
		{
			rError("Memory::Merge('%s') failed with errno %d",
				m_name.c_str(), errno);

			// Error happend, don't try to continue. But also
			// don't forget to release allocated memory...

			m_LinearMap.truncate(0);

			m_FileSize = 0;
			m_FileSizeSet = false;
			return -1;
		}
	}

	// Let the upper layer know the real file size.

	Parent::truncate(name, m_FileSize);

	return 0;
}

int Memory::open(const char *name, int flags)
{
	int r = Parent::open(name, flags);

	if ((m_refs == 1) && (m_FileSizeSet == false))
	{
		struct stat st;

		int r = Parent::getattr(name, &st);
		if (r != 0)
		{
			rDebug("Memory::open('%s') failed with errno: %d",
				m_name.c_str(), errno);

			Parent::release(name);
			return -1;
		}
		m_FileSize = st.st_size;
		m_FileSizeSet = true;
	}

	return r;
}

int Memory::release(const char *name)
{
	int rm = 0;
	int rp;

	if (m_refs == 1)
	{
		// Store all cached data in ram to file only
		// if this is last instance...
		//
		rm = merge(name);

		if (m_TimeSet == true)
		{
			m_TimeSet = false;
			Parent::utimens(name, m_Time);
		}
		m_FileSize = 0;
		m_FileSizeSet = false;
	}

	// Release lower file.
	//
	rp = Parent::release(name);

	// Return any error...
	//
	return rm ? rm : rp;
}

int Memory::unlink(const char *name)
{
	assert(m_name == name);

	int r = Parent::unlink(name);
	if (r == 0)
	{
		// Lower file deleted, release all memory
		// buffers allocated so far...
		// 
		m_LinearMap.truncate(0);

		m_FileSize = 0;
		m_FileSizeSet = false;
	}
	else
	{
		rDebug("Memory::unlink('%s') failed with errno: %d",
			m_name.c_str(), errno);
	}
	return r;
}

int Memory::truncate(const char *name, off_t size)
{
	assert(m_name == name);

	m_TimeSet = false;

	int r = Parent::truncate(name, size);
	if (r == 0)
	{
		m_LinearMap.truncate(size);

		m_FileSize = size;
		m_FileSizeSet = true;
	}
	else
	{
		rWarning("Memory::truncate('%s', 0x%lx) failed with errno: %d",
		          m_name.c_str(), (long int) size, errno);
	}
	return r;
}

int Memory::getattr(const char *name, struct stat *st)
{
	int r = Parent::getattr(name, st);

	rDebug("Memory::getattr(%p) m_FileSize: 0x%lx, m_FileSizeSet: %d",
		(void *) this, (long int) m_FileSize, m_FileSizeSet);

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

	while (m_LinearMap.erase(&offset, &buf, &size, force) == true)
	{
		rDebug("Memory::write(bool %d) | offset: 0x%lx, size: 0x%lx",
			force, (unsigned long) offset, (unsigned long) size);

		len = Parent::write(buf, size, offset);
		delete[] buf;
		if (len == -1)
			return -1;
	}
	return 0;
}

ssize_t Memory::write(const char *buf, size_t size, off_t offset)
{
	rDebug("Memory::write(%s) | m_FileSize: 0x%lx, size: 0x%lx, offset: 0x%lx",
			m_name.c_str(), (long int) m_FileSize, (long int) size, (long int) offset);

	m_TimeSet = false;

	if ((m_FileSize == offset) && FileUtils::isZeroOnly(buf, size))
	{
		rDebug("Memory::write(%s) | Full of zeroes only", m_name.c_str());

		assert(m_FileSizeSet == true);
		assert(size > 0);
		m_FileSize = offset + size;
	}
	else
	{
		// Store buffer to memory in LinearMap.
		//
		if (m_LinearMap.put(buf, size, offset) == -1)
			return -1;

		assert(m_FileSizeSet == true);
		assert(size > 0);
		m_FileSize = max(m_FileSize, (off_t) (offset + size));

		// Try to write a block to disk if appropriate.
		// 
		int r = write(false);
		if (r == -1)
			return r;
	}

	return size;
}

ssize_t Memory::readFullParent(char * &buf, size_t &len, off_t &offset) const
{
	ssize_t r = Parent::read(buf, len, offset);

	rDebug("Memory::readFullParent(%s) | Parent::read(1) returned 0x%lx",
		m_name.c_str(), (long int) r);

	if (r == -1)
		return -1;

	buf    += r;
	offset += r;
	len    -= r;

	assert (m_FileSize >= offset);

	if (len > 0)
	{
		ssize_t tmp = min(m_FileSize - offset, (off_t) len);

		assert(tmp >= 0);
		assert(tmp <= len);

		memset(buf, 0, tmp);

		buf    += tmp;
		offset += tmp;
		len    -= tmp;
	}

	return 0;
}

ssize_t Memory::readParent(char * &buf, size_t &len, off_t &offset, off_t block_offset) const
{
	size_t size = std::min(len, (size_t) (block_offset - offset));
	size_t osize = size;

	ssize_t r = readFullParent(buf, size, offset);
	if (r == -1)
		return -1;

	assert (size == 0);
	len -= (osize - size);

	return 0;
}

void Memory::copyFromBlock(char * &buf, size_t &len, off_t &offset, char *block, size_t block_size, off_t block_offset) const
{
	assert (offset >= block_offset);
	size_t block_loffset = offset - block_offset;
	assert (block_size >= block_loffset);
	size_t size = std::min(len, block_size - block_loffset);
	assert (size >= 0);

	memcpy(buf, block + block_loffset, size);

	buf    += size;
	offset += size;
	len    -= size;
}

ssize_t Memory::read(char *buf, size_t size, off_t offset) const
{
	size_t	 osize = size;
	size_t	 len = size;

	rDebug("Memory::read(%s) | m_FileSize: 0x%lx, offset: 0x%lx, size: 0x%lx",
			m_name.c_str(), (long int) m_FileSize, (long int) offset, (long int) size);

	assert(m_FileSizeSet == true);

	if (offset > m_FileSize)
	{
		rDebug("Memory::read(%s) | offset > m_FileSize, return: 0x%lx",
		       m_name.c_str(), (long int) 0);
		return 0;
	}
	while (len > 0)
	{
		char	*block;
		size_t	 block_size;
		off_t    block_offset = m_LinearMap.get(offset, &block, &block_size);

		rDebug("Memory::read(%s) | offset: 0x%lx, block_offset: 0x%lx, block_size: 0x%lx, block: %p",
			m_name.c_str(), (long int) offset, (long int) block_offset, (long int) block_size, block);

		if (block_offset == -1)
		{
			// Block not found, there isn't any block higher too.
			// Try to read the data form the Parent. Don't forget
			// to fill up with zeros to m_FileSize if Parent read
			// returns less.

			ssize_t r = readFullParent(buf, len, offset);
			if (r == -1)
				return -1;
			break;
		}

		if (block_offset > offset)
		{
			// Block found, but it covers higher offset. Read from the
			// parent. If parent returns less, fill the gap with zeros.

			ssize_t r = readParent(buf, len, offset, block_offset);
			if (r == -1)
				return -1;
		}
		else
		{
			// Block found, it covers the offset. Take as much as possible
			// from the block.

			copyFromBlock(buf, len, offset, block, block_size, block_offset);
		}
	}

	rDebug("Memory::read(%s) | return: 0x%lx",
	        m_name.c_str(), (long int) (osize - len));

	return osize - len;
}

int Memory::utimens(const char *name, const struct timespec tv[2])
{
	m_TimeSet = true;
	m_Time[0] = tv[0];
	m_Time[1] = tv[1];
	return 0;
}


