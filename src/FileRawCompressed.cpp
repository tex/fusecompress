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

FileRawCompressed::FileRawCompressed(const FileHeader &fh, off_t length) :
	m_fd (-1),
	m_fh (fh),
	m_length (length)
{
	// Limit m_length at least to FileHeader::HeaderSize - this
	// is for new files to reserve space for header...

	assert(m_length == 0 || m_length >= FileHeader::MaxSize);

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

/* m_fh must be correct. m_length may be changed. */
void FileRawCompressed::restore(LayerMap &lm, int fd)
{
	rDebug("%s: fd: %d", __PRETTY_FUNCTION__, fd);

	nonclosable_file_descriptor file(fd);
	file.seek(m_fh.index, ios_base::beg);

	filtering_istream in;
	m_fh.type.push(in);
	in.push(file);

	portable_binary_iarchive pba(in);
	pba >> lm;

	// Optimization on size. Overwrite the index during
	// next write.

	if (file.seek(0, ios_base::cur) == file.seek(0, ios_base::end))
		m_length = m_fh.index;
}

void FileRawCompressed::store(FileHeader& fh, const LayerMap& lm, int fd)
{
	rDebug("%s: fd: %d", __PRETTY_FUNCTION__, fd);
{
	nonclosable_file_descriptor file(fd);
	file.seek(0, ios_base::beg);

	filtering_ostream out;
	out.push(file);

	portable_binary_oarchive pba(out);
	pba << fh;
}
{
	nonclosable_file_descriptor file(fd);
	file.seek(fh.index, ios_base::beg);

	filtering_ostream out;
	fh.type.push(out);
	out.push(file);

	portable_binary_oarchive pba(out);
	pba << lm;
}
}

int FileRawCompressed::open(int fd)
{
	assert (m_fd == -1);
	m_fd = fd;

	assert (m_length >= FileHeader::MaxSize);
	if (m_length == FileHeader::MaxSize)
	{
		// Empty file, no LayerMap there.
		return m_fd;
	}
	
	assert (m_fh.index != 0);
try {
	restore(m_lm, m_fd);
}
catch (...)
{
	int tmp = errno;
	rError("%s: Failed to restore LayerMap (%s)", __PRETTY_FUNCTION__, strerror(errno));
	errno = tmp;
	m_fd = -1;
	return -1;
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

	m_fh.index = m_length;

try {
	store(m_fh, m_lm, m_fd);
}
catch (...)
{
	int tmp = errno;
	rError("%s: Failed to store FileHeader and/or LayerMap (%s)", __PRETTY_FUNCTION__, strerror(errno));
	errno = tmp;
	return -1;
}
	return 0;
}

int FileRawCompressed::close()
{
	m_lm.Truncate(0);
	m_fd = -1;

	return 0;
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

			delete[] b;

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

	Block *bl = NULL;
try {
	// Append a new Block to the file.

	bl = new Block(g_CompressionType);

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

	// Destroying the object 'out' causes all filters to
	// flush.
}
	// Update raw length of the file.
	// 
	m_length = file.seek(0, ios_base::end);

	bl->clength = m_length - bl->coffset;

	m_lm.Put(bl);

} catch (...)
{
	delete bl;
	return -1;
}
	
	// Update length of the file.
	//
	assert(size > 0);
	if (m_fh.size < offset + (off_t) size)
	{
		m_fh.size = offset + size;
	}

	store(m_fd);

	// Ok, size of the file on the disk is double than
	// it would be uncompressed. That's bad, defragment the
	// file.

	if (m_length > m_fh.size * 2)
	{
		DefragmentFast();
	}

	return size;
}

int FileRawCompressed::truncate(const char *name, off_t size)
{
	int r = 0;
	bool openedHere = false;

	rDebug("%s: m_fd: %d, size:0x%llx", __PRETTY_FUNCTION__, m_fd, size);

	if (m_fd == -1)
	{
		int fd = FileUtils::open(name);
		if (fd < 0)
		{
			return -1;
		}
		if (open(fd) == -1)
		{
			::close(fd);
			return -1;
		}
		if (m_fd < 0)
		{
			::close(fd);
			return -1;
		}
		openedHere = true;
	}
	rDebug("%s: m_fd: %d", __PRETTY_FUNCTION__, m_fd);

	m_fh.size = size;
	m_lm.Truncate(size);

	// Truncate to zero lenght is the only what
	// we can implement easily.
	//
	if (size == 0)
	{
		m_length = FileHeader::MaxSize;
		if (::ftruncate(m_fd, m_length) == -1)
		{
			goto exit;
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
	}
	else
	{
		DefragmentFast();
	}
exit:
	if (openedHere)
	{
		::close(m_fd);
		m_fd = -1;
	}

	rDebug("FileRawCompressed::truncate");
	return r;
}

void FileRawCompressed::DefragmentFast()
{
//	return;

	FileRememberTimes frt(m_fd);

	int tmp_fd;
	off_t tmp_offset;

	// Prepare a temporary file.
{
	char tmp_name[] = "./XXXXXX";

	tmp_fd = mkstemp(tmp_name);
	if (tmp_fd < 0)
	{
		assert(errno != EINVAL);	// This would be a programmer error
		rError("%s: Cannot open temporary file.", __PRETTY_FUNCTION__);
		return;
	}
	::unlink(tmp_name);

	// Reserve space for a FileHeader.

	tmp_offset = FileHeader::MaxSize;
}
	off_t to_write = 0;
	off_t offset = 0;
	off_t size = m_fh.size;

	LayerMap			tmp_lm;
	FileHeader			tmp_fh;
{
	nonclosable_file_descriptor	file(m_fd);
	nonclosable_file_descriptor	tmp_file(tmp_fd);

	while (size > 0)
	{
		off_t length = 0;

		Block bl;

		if (m_lm.Get(offset, bl, length) == false)
		{
			// Block not found. There also is no block on a upper
			// offset.

			break;
		}

		if (length == 0)
		{
			// Block doesn't exists on the offset, but there is
			// a Block on the bigger offset.

			off_t tmp = (bl.offset - offset);
			size -= tmp;
			offset += tmp;
			continue;
		}

		Block *block = new (std::nothrow) Block(bl);
		if (!block)
		{
			::close(tmp_fd);
			rError("%s: No space to allocate a Block", __PRETTY_FUNCTION__);
			return;
		}

		char *buf, *buf_write;

		filtering_ostream out;

		if ((length == block->length) && (block->length == block->olength))
		{
			filtering_istream in;
			in.push(file);

			// The whole block is in use, just copy raw block.

			assert(offset == block->offset);

			buf = buf_write = new char[block->clength];

			file.seek(block->coffset, ios_base::beg);
			io::read(in, buf, block->clength);
			to_write = block->clength;
		}
		else
		{
			filtering_istream in;
			block->type.push(in);

			file.seek(block->coffset, ios_base::beg);
			in.push(file);

			block->type.push(out);

			// Just a part of a block is in use, decompress whole block
			// and create a new block covering only used part.

			buf = buf_write = new char[block->length];

			io::read(in, buf, block->length);
			to_write = length;

			block->offset += block->length - length;
			block->length = length;
			block->olength = length;

			buf_write += block->length - length;
		}

		block->coffset = tmp_offset;

		tmp_file.seek(tmp_offset, ios_base::beg);
		out.push(tmp_file);
		io::write(out, buf_write, to_write);
		out.reset();	// Flush the buffers

		tmp_offset = tmp_file.seek(0, ios_base::end);
		block->clength =  tmp_offset - block->coffset;

		delete[] buf;

		tmp_lm.Put(block);

		offset += length;
		size -= length;
	}

	tmp_fh.index = tmp_offset;
	tmp_fh.size = m_fh.size;

	store(tmp_fh, tmp_lm, tmp_fd);

	m_length = tmp_file.seek(0, ios_base::end);
}
	// m_fd contains original file.
	// tmp_fd contains defragmented file.

	FileUtils::copy(tmp_fd, m_fd);
	::close(tmp_fd);

	// m_fd contains only defragmented file.
	// tmp_fd no longer exists on the filesystem.
	// tpm_fh contains complete information.
	// tmp_lm contains complete information.

	m_lm.acquire(tmp_lm);
	m_fh.acquire(tmp_fh);
}

bool FileRawCompressed::isTransformableToFileRawNormal()
{
	rDebug("FileRawCompressed::isTransformableToFileRawNormal");

	if (m_fd == -1)
	{
		return false;
	}

	if (m_length != FileHeader::MaxSize)
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

