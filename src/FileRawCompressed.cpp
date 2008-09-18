#include <algorithm>
#include <cassert>
#include <errno.h>
#include <rlog/rlog.h>
#include <cstdlib>
#include <cstring>
#include <strings.h>

//#include <boost/serialization/binary_object.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/write.hpp>

#include <boost/iostreams/device/nonclosable_file_descriptor.hpp>

#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "CompressionType.hpp"
#include "FileRememberTimes.hpp"
#include "FileRawNormal.hpp"
#include "FileRawCompressed.hpp"
#include "FileUtils.hpp"

using namespace std;
using namespace boost::iostreams;

namespace io = boost::iostreams;
namespace se = boost::serialization;

extern CompressionType g_CompressionType;

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
	m_length (length),
	m_store (false)
{
	// Limit m_length at least to FileHeader::HeaderSize - this
	// is for new files to reserve space for header...
	//
	if (m_length == 0)
	{
		m_length = FileHeader::MaxSize;
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

	if (m_length <= FileHeader::MaxSize)
	{
		return m_fd;
	}
	
	// File is non-empty, so there must be index
	// there.
	// 
//	assert (m_fh.index != 0);
try {
	filtering_istream in;
	in.set_auto_close(false);
	file_descriptor file(m_fd);

	m_fh.type.push(in);
	in.push(file);

	portable_binary_iarchive pba(in);
	file.seek(m_fh.index, ios_base::beg);
	pba >> m_lm;

//	in.pop();
//	in.pop();
//	in.reset();

	if (file.seek(0, ios_base::cur) == file.seek(0, ios_base::end))
		m_length = m_fh.index;

} catch (...)
{
	m_fd = -1;
}		
	return m_fd;
}

int FileRawCompressed::store(int fd)
{
	FileRememberTimes frt(fd);

	// Append new index to the end of the file
	// => forget the old one...
	//
	// Set a new position of the index in the FileHeader.
	// 
	m_fh.index = m_length;

	// Store index to a file and update raw length of
	// the file.
	//
try {
{
	filtering_ostream out;
//	out.set_auto_close(false);
	nonclosable_file_descriptor file(m_fd);

	out.push(file);

	portable_binary_oarchive pba(out);

	file.seek(0, ios_base::beg);
	pba << m_fh;
//	out.strict_sync();
}
{
	filtering_ostream out;
//	out.set_auto_close(false);
	nonclosable_file_descriptor file(m_fd);

	m_fh.type.push(out);
	out.push(file);

	portable_binary_oarchive pba(out);

	file.seek(m_fh.index, ios_base::beg);
	pba << m_lm;
//	out.pop();
//	out.pop();
//	out.reset();
}
}
catch (...)
{
	return -1;
}
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

//	cout << __PRETTY_FUNCTION__ << "m_fd: " << m_fd << ", m_store: " << m_store << endl;

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

//	cout << __FUNCTION__ << "; size: 0x" << hex << size << ", offset: 0x" << hex << offset << endl;
//	cout << "m_fh.size: 0x" << hex << m_fh.size << endl;

	assert(size >= 0);
	if (offset + (off_t) size > m_fh.size)
	{
		if (m_fh.size > offset)
			size = m_fh.size - offset;
		else
			size = 0;
	}
	osize = size;

//	cout << __FUNCTION__ << "; size: 0x" << hex << size << endl;

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
			std::cout << block << std::endl;
try {
			filtering_istream in;
			nonclosable_file_descriptor file(m_fd);

			file.seek(block.coffset, ios_base::beg);

			block.type.push(in);
			in.push(file);

			char *b = new char[block.length];

			// Optimization: read only as much bytes as neccessary.

			r = min((off_t)(size), len);
			int l = offset - block.offset + r;
			assert(l <= block.length);

			io::read(in, b, l);

			memcpy(buf, b + offset - block.offset, r);

			delete b;

//			in.pop();
//			in.pop();
//			in.reset();

} catch (...) { return -1; }

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

//	cout << "return: 0x" << hex << osize - size << endl;

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

	rDebug("FileRawCompressed::write, %d", m_length);
try {
	// Append a new Block to the file.

	Block *bl = new Block(g_CompressionType);

	bl->offset = offset;
	bl->length = size;
	bl->olength = size;
	bl->coffset = m_length;

	nonclosable_file_descriptor file(m_fd);
	file.seek(bl->coffset, ios_base::beg);
{
	filtering_ostream out;

	bl->type.push(out);
	out.push(file);

	io::write(out, buf, bl->length);

//	out.pop();
//	out.pop();
//	out.reset();

	// Destroying the object 'out' causes all filters to
	// flush.
}
	// Update raw length of the file.
	// 
	m_length = file.seek(0, ios_base::end);

	m_lm.Put(bl);

} catch (...) { return -1; }
	
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

	if (m_fd == -1)
	{
		open(FileUtils::open(name));
		bopen = true;
	}

	if (m_fd < 0)
		return -1;
	
	m_fh.size = size;

	m_lm.Truncate(size);

	// Truncate to zero lenght is the only what
	// we can implement easily.
	//
	if (size == 0)
	{
		m_length = FileHeader::MaxSize;

		r = ::truncate(name, m_length);
	}

	r = store(m_fd);
	if (r == -1)
	{
		int tmp = errno;
		rError("FileRawCompressed::truncate Cannot write "
		       "index to file '%s', error: %s",
		       name, strerror(tmp));
		errno = tmp;
	}

	if (size != 0)
	{
		Defragment();
	}

	if (bopen)
	{
		::close(m_fd);
		m_fd = -1;
	}

	rDebug("FileRawCompressed::truncate");

	return r;
}

void FileRawCompressed::Defragment()
{
}

bool FileRawCompressed::isTransformableToFileRawNormal()
{
	rDebug("FileRawCompressed::isTransformableToFileRawNormal");

	if (m_fd == -1)
	{
		return false;
	}

	if (m_store && m_length != FileHeader::MaxSize)
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

