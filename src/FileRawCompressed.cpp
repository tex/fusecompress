#include "FileRememberTimes.hpp"
#include "FileRawNormal.hpp"
#include "FileRawCompressed.hpp"
#include "TransformTable.hpp"
#include "FileUtils.hpp"

#include <algorithm>
#include <cassert>
#include <errno.h>
#include <rlog/rlog.h>
#include <cstdlib>
#include <cstring>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

extern TransformTable *g_TransformTable;

// Sets the type of transformation applied to index.
// We use only lzo compression to compress index for now.
// 
// Improvement?
// Have a policy that index is transformed with
// the same type of transformation as the data.
// 
FileRawCompressed::FileRawCompressed(const FileHeader &fh, off_t length) :
	m_fd (-1),
	m_fh (fh),
	m_lm (g_TransformTable->getTransform(3)),
	m_length (length),
	m_store (false)
{
	assert ((m_length == 0) || (m_length >= FileHeader::HeaderSize));

	// Limit m_length at least to FileHeader::HeaderSize - this
	// is for new files to reserve space for header...
	//
	if (m_length == 0)
	{
		m_length = FileHeader::HeaderSize;
	}
}

FileRawCompressed::~FileRawCompressed()
{
	if (m_fd != -1)
	{
		close();
	}
}

int FileRawCompressed::open(int fd)
{
	assert (m_fd == -1);

	m_fd = fd;

	// Assign transformation object according to the
	// type of the transformation stored in the
	// header of the file.
	//
	m_transform = g_TransformTable->getTransform(m_fh.type);

	if (m_length == FileHeader::HeaderSize)
	{
		return m_fd;
	}
	
	// File is non-empty, so there must be index
	// there.
	// 
	assert (m_fh.index != 0);
	
	off_t r = m_lm.restore(m_fd, m_fh.index);

	if (r == -1)
	{
		// There should be index, but it cannot
		// be restored => I/O error?
		//
		m_fd = -1;
	}
	else
	{
		if (r == m_length)
		{
			// Index is stored at the end of the file.
			// 
			off_t index_length = r - m_fh.index;

			m_length -= index_length;
		}
		
	}

	return m_fd;
}

int FileRawCompressed::store(int fd)
{
	off_t r;

	FileRememberTimes frt(fd);

	// Append new index to the end of the file
	// => forget the old one...
	//
	// Set a new position of the index in the FileHeader.
	// 
	m_fh.index = m_length;
cout << "1:" << m_fh.index << endl;
	// Store index to a file and update raw length of
	// the file.
	//
	r = m_lm.store(fd, m_fh.index);
	switch (r) {
	case -1:
		return -1;
	case  0:
		r = m_length;
		m_fh.index = 0;
	}
cout << "2:" << m_fh.index << endl;
	
	m_length = r;

	// Sync the header, header is always at the begining
	// of the file.
	//
	r = m_fh.store(fd, 0);
	if (r == -1)
		return -1;

	return 0;
}

int FileRawCompressed::store(const char *name)
{
	int	fd;
	off_t	r = 0;
	
	fd = FileUtils::open(name);
	if (fd < 0)
		return -1;
	
	r = store(fd);
	
	::close(fd);

	return r;
}

int FileRawCompressed::close()
{
	off_t r = 0;

	cout << __PRETTY_FUNCTION__ << "m_fd: " << m_fd << ", m_store: " << m_store << endl;

	if (m_fd == -1)
	{
		return 0;
	}

	if (m_store)
	{
		// File was changed, write index to the file.
		//
		r = store(m_fd);
	}

	m_fd = -1;

	return r;
}

ssize_t FileRawCompressed::read(char *buf, size_t size, off_t offset)
{
	Block	 block;
	size_t	 osize;
	off_t	 len;
	off_t	 r;

	cout << __FUNCTION__ << "; size: 0x" << hex << size << ", offset: 0x" << hex << offset << endl;
	cout << "m_fh.size: 0x" << hex << m_fh.size << endl;

	assert(size >= 0);
	if (offset + (off_t) size > m_fh.size)
	{
		if (m_fh.size > offset)
			size = m_fh.size - offset;
		else
			size = 0;
	}
	osize = size;

	cout << __FUNCTION__ << "; size: 0x" << hex << size << endl;

	while (size > 0)
	{
		if (!m_lm.Get(offset, block, len))
		{
			// Block not found. There also is no block on a upper
			// offset.
			//
			memset(buf, 0, size);
			size = 0;
			break;
		}

		if (len)
		{
			// Block covers the offset, we can read len bytes
			// from it's de-compressed stream...
			//
			r = m_transform->restore(m_fd, &block, offset, buf,
			                         min((off_t) (size), len));
			if (r == -1)
				return -1;

			buf += r;
			offset += r;
			size -= r;
		}
		else
		{
			// Block doesn't exists on the offset, but there is
			// a Block on the bigger offset. Fill the gap with
			// zeroes...
			//
			r = min(block.offset - offset, (off_t) (size));

			memset(buf, 0, r);

			buf += r;
			offset += r;
			size -= r;
		}
	}

	cout << "return: 0x" << hex << osize - size << endl;

	return osize - size;
}

ssize_t FileRawCompressed::write(const char *buf, size_t size, off_t offset)
{
	if (m_fd == -1)
	{
		errno = -EBADF;
		return -1;
	}
//	assert (m_fd != -1);

	rDebug("FileRawCompressed::write");
	
	m_store = true;

	// Append a new Block to the file.
	//
	Block *bl = m_transform->store(m_fd, offset, m_length, buf, size);
	if (!bl)
		return -1;
	
	m_lm.Put(bl);
	
	// Update raw length of the file.
	// 
	m_length += bl->clength;

	// Update length of the file.
	//
	assert(size > 0);
	if (m_fh.size < offset + (off_t) size)
	{
		m_fh.size = offset + size;
	}

	store(m_fd);

	return size;
}

int FileRawCompressed::truncate(const char *name, off_t size)
{
	int r = 0;
	bool bopen = false;

	rDebug("%s", __PRETTY_FUNCTION__);

	if (m_fd == -1)
	{
		rDebug("Calling open(%s)", name);

		open(FileUtils::open(name));
		bopen = true;

		if (m_fd == -1)
		{
			rDebug("Error");
			return -1;
		}
	}
	
	m_fh.size = size;

	m_lm.Truncate(size);

	// Truncate to zero lenght is the only what
	// we can implement easily.
	//
	if (size == 0)
	{
		m_length = FileHeader::HeaderSize;

		r = ::truncate(name, m_length);
	}

	r = store(m_fd);

	if (bopen)
	{
		::close(m_fd);
		m_fd = -1;
	}

	return r;
}

bool FileRawCompressed::isTransformableToFileRawNormal()
{
	rDebug("FileRawCompressed::isTransformableToFileRawNormal");

	if (m_fd == -1)
	{
		return false;
	}

	if (m_store && m_length != FileHeader::HeaderSize)
	{
		// FileHeader should be written to the disk sometime,
		// and length of the file is different than length
		// of the FileHeader so there must be data already
		// present there...
		// 
		return false;
	}

	return true;
}

FileRawNormal *FileRawCompressed::TransformToFileRawNormal(FileRaw *pFr)
{
	FileRawNormal		*pFrn;
	FileRawCompressed	*pFrc;

	rDebug("FileRawCompressed::TransformToFileRawNormal");

	pFrc = reinterpret_cast<FileRawCompressed *>(pFr);

	::ftruncate(pFrc->m_fd, 0);
	
	pFrn = new (std::nothrow) FileRawNormal(pFrc->m_fd);
	if (!pFrn)
	{
		rError("No memory to allocate object of FileRawNormal class");
		abort();
	}

	// Set m_fd to -1 to pretend the destructor from calling
	// close method that would may store FileHeader to the
	// file.
	//
	pFrc->m_fd = -1;

	// Now's the time to deallocate FileRawCompressed instance.
	// 
	delete pFrc;

	// Return pointer of FileRawNormal instance to the caller.
	//
	return pFrn;
}

