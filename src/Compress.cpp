#include <errno.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <rlog/rlog.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/scoped_array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/nonclosable_file_descriptor.hpp>

#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>

#include "config.h"

#if defined(HAVE_ATTR_XATTR_H)
#  include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
#  include <sys/xattr.h>
#endif

#include "Compress.hpp"
#include "FileUtils.hpp"
#include "FileRememberTimes.hpp"
#include "FileManager.hpp"

#include "CompressedMagic.hpp"

namespace io = boost::iostreams;
namespace se = boost::serialization;

extern CompressedMagic	 g_CompressedMagic;
extern CompressionType	 g_CompressionType;
extern FileManager	*g_FileManager;

Compress::Compress(const struct stat *st, const char *name) :
	Parent (st, name),
	m_RawFileSize (st->st_size)
{
	if (m_RawFileSize == 0)
	{
		// New empty file, default strategy
		// is to compress a file.
		//
		m_IsCompressed = true;

		// New empty file, will be compressed,
		// reserve space for a FileHeader.
		//
		m_RawFileSize = FileHeader::MaxSize;
	}
	else if (m_RawFileSize < FileHeader::MaxSize)
	{
		// Nonempty file with length smaller
		// than length of the FileHeader has to
		// be uncompressed.
		//
		m_IsCompressed = false;
	}
	else
	{
		// This is a constructor, no one could
		// call the open() function yet.

		assert(m_fd == -1);

		try {
			// Try to restore the FileHeader.
			//
			restore(m_fh, name);

			m_IsCompressed = m_fh.isValid();
		}
		catch (...) { m_IsCompressed = false; }
	}

	if (m_IsCompressed)
	{
		rDebug("C (%s), raw/user 0x%lx/0x%lx bytes",
				name, (long int) m_RawFileSize, (long int) m_fh.size);
	}
	else
	{
		rDebug("N (%s), 0x%lx bytes", name, m_RawFileSize);
	}
}

Compress::~Compress()
{
	rDebug("%s, %s", __PRETTY_FUNCTION__, m_name.c_str());

	assert(m_refs == 0);
}

int Compress::unlink(const char *name)
{
	rDebug("%s name: %s", __FUNCTION__, name);

	m_RawFileSize = 0;

	if (m_IsCompressed)
	{
		m_lm.Truncate(0);	// Free allocated memory
	}

	return PARENT_COMPRESS::unlink(name);
}

int Compress::truncate(const char *name, off_t size)
{
	rDebug("%s name: %s, m_IsCompressed: %d, size: %llx", __FUNCTION__, name, m_IsCompressed, size);

	if (!m_IsCompressed)
	{
		off_t r = ::truncate(name, size);
		if (r >= 0)
		{
			// Update raw file size if success.
			//
			m_RawFileSize = r;
		}
		return r;
	}

	int	r          = 0;
	bool	openedHere = false;

	if (m_fd == -1)
	{
		if (open(name, O_RDONLY) == -1)
		{
			return -1;
		}
		openedHere = true;
	}

	m_fh.size = size;
	m_lm.Truncate(size);

	// Truncate to zero lenght is the only what
	// we can implement easily for compressed file.
	//
	if (size == 0)
	{
		if (::ftruncate(m_fd, FileHeader::MaxSize) != 0)
		{
			goto exit;
		}

		m_RawFileSize = FileHeader::MaxSize;

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
		release(name);
	}

	return r;
}

int Compress::getattr(const char *name, struct stat *st)
{
	int r;

	rDebug("%s name: %s, m_IsCompressed: %d, m_fh.size: 0x%llx", __FUNCTION__, name, m_IsCompressed, m_fh.size);

	r = PARENT_COMPRESS::getattr(name, st);

	if (m_IsCompressed)
	{
		assert(m_fh.isValid() == true);

		st->st_size = m_fh.size;
	}

	return r;
}

int Compress::open(const char *name, int flags)
{
	int r;

	r = PARENT_COMPRESS::open(name, flags);

	if (m_IsCompressed && (m_refs == 1))
	{
		assert (m_RawFileSize >= FileHeader::MaxSize);

		// Speed optimization for new empty (thus compressed)
		// file.

		if (m_RawFileSize == FileHeader::MaxSize)
		{
			// Empty file, no LayerMap there.
			//
			return m_fd;
		}
		
		assert (m_fh.index != 0);

		try {
			restore(m_lm, m_fd);
		}
		catch (...)
		{
			rError("%s: Failed to restore LayerMap (%s)", __PRETTY_FUNCTION__);
			release(name);
			errno = -EIO;
			return -1;
		}
	}

	rDebug("Compress::open m_refs: %d", m_refs);

	return r;
}

int Compress::release(const char *name)
{
	if (m_IsCompressed && (m_refs == 1))
	{
		m_lm.Truncate(0);
	}

	int r = PARENT_COMPRESS::release(name);

	rDebug("Compress::release m_refs: %d", m_refs);

	return r;
}

off_t Compress::writeCompressed(LayerMap& lm, off_t offset, off_t coffset, const char *buf, size_t size, int fd)
{
	assert(coffset >= FileHeader::MaxSize);

	rDebug("offset: 0x%x, coffset: 0x%x, size: 0x%x", offset, coffset, size);

	Block *bl = NULL;

	try {
		// Append a new Block to the file.

		bl = new Block(g_CompressionType);

		bl->offset = offset;
		bl->coffset = coffset;
		bl->length = size;
		bl->olength = size;

		io::nonclosable_file_descriptor file(fd);
		file.seek(bl->coffset, ios_base::beg);
		{
			io::filtering_ostream out;

			bl->type.push(out);
			out.push(file);

			io::write(out, buf, bl->length);

			// Destroying the object 'out' causes all filters to
			// flush.
		}
		// Update raw length of the file.
		// 
		coffset = file.seek(0, ios_base::end);

		bl->clength = coffset - bl->coffset;
	}
	catch (...)
	{
		rError("Failed to add a new Block to the file.");

		delete bl;
		return -1;
	}
	
	assert(bl != NULL);
	lm.Put(bl);

	rDebug("length: 0x%x", coffset);

	return coffset;
}

ssize_t Compress::write(const char *buf, size_t size, off_t offset)
{
	// Spurious call to write when file has not been opened
	// happened during testing...

	if (m_fd == -1)
	{
		errno = -EBADF;
		return -1;
	}
	assert (m_fd != -1);

	rDebug("Compress::write size: 0x%x, offset: 0x%llx", (unsigned int) size,
			(long long int) offset);

	// We have an oppourtunity to decide whether we really
	// want to compress the file. We use file magic library
	// to detect mime type of the file to decide the compress
	// strategy.
 
	if ((m_IsCompressed == true) &&
	    (offset == 0) &&
	    (m_RawFileSize == FileHeader::MaxSize) &&
	    (g_CompressedMagic.isNativelyCompressed(buf, size)))
	{
		m_IsCompressed = false;
	}

	if (m_IsCompressed == false)
	{
		ssize_t ret = pwrite(m_fd, buf, size, offset);
		if (ret > 0)
		{
			m_RawFileSize = max(m_RawFileSize, offset + ret);
		}
		return ret;
	}
	else
	{
		off_t rawFileSize = writeCompressed(m_lm, offset, m_RawFileSize, buf, size, m_fd);
		if (rawFileSize == -1)
			return -1;
		m_RawFileSize = rawFileSize;

		// Update apparent length of the file.
		//
		assert(size > 0);
		m_fh.size = max(m_fh.size, (off_t) (offset + size));

		rDebug("%s m_fh_size: 0x%llx", __PRETTY_FUNCTION__, m_fh.size);

		store(m_fd);

		// If size of the file on the disk is double than
		// it would be uncompressed, defragment the file.
		// Only if raw file size is bigger than 4096; the
		// size of a sector on the lower filesystem.

//		if (m_RawFileSize > 4096 && m_RawFileSize > m_fh.size * 2)
		{
			DefragmentFast();
		}

		return size;
	}
}

/* m_fh.size, m_lm */
ssize_t Compress::readCompressed(char *buf, size_t size, off_t offset, int fd)
{
	Block	 block;
	size_t	 osize;
	off_t	 len;

	if (offset + (off_t) size > m_fh.size)
	{
		if (m_fh.size > offset)
		{
			size = m_fh.size - offset;
		} else
			size = 0;
	}
	osize = size;

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

			off_t r;

			try {
				io::nonclosable_file_descriptor file(fd);
				file.seek(block.coffset, ios_base::beg);

				io::filtering_istream in;
				block.type.push(in);
				in.push(file);

				boost::scoped_array<char> buf_tmp(new char[block.length]);

				// Optimization: read only as much bytes as neccessary.

				r = min((off_t)(size), len);

				off_t not_needed = offset - block.offset;
				off_t must_read = not_needed + r;
				assert(must_read <= block.length);

				io::read(in, buf_tmp.get(), must_read);
				memcpy(buf, buf_tmp.get() + not_needed, r);
			}
			catch (...)
			{
				rError("Block read failed: block.offset:%lld, block.coffset:%lld, block.length: %lld, block.clength: %lld",
					block.offset, block.coffset, block.length, block.clength);
				return -1;
			}

			buf += r;
			offset += r;
			size -= r;
		}
		else
		{
			off_t r;

			// Block doesn't exists on the offset, but there is
			// a Block on the bigger offset. Fill the gap with
			// zeroes...

			r = min(block.offset - offset, (off_t) (size));

			memset(buf, 0, r);

			buf += r;
			offset += r;
			size -= r;
		}
	}

	return osize - size;
}

ssize_t Compress::read(char *buf, size_t size, off_t offset)
{
	assert (m_fd != -1);
	assert (size >= 0);

	rDebug("Compress::read size: 0x%x, offset: 0x%llx", (unsigned int) size,
			(long long int) offset);

	if (m_IsCompressed == false)
	{
		return pread(m_fd, buf, size, offset);
	}
	else
	{
		return readCompressed(buf, size, offset, m_fd);
	}
}

void Compress::restore(FileHeader& fh, int fd)
{
	rDebug("Using nonclosable_file_descriptor");

	io::nonclosable_file_descriptor file(m_fd);
	io::filtering_istream in;
	in.push(file);
	portable_binary_iarchive pba(in);
	pba >> fh;
}

void Compress::restore(FileHeader& fh, const char *name)
{
	rDebug("Using file name");

	ifstream file(name);
	io::filtering_istream in;
	in.push(file);
	portable_binary_iarchive pba(in);
	pba >> fh;
}

/* m_fh must be correct. m_length may be changed. */
void Compress::restore(LayerMap &lm, int fd)
{
	rDebug("%s: fd: %d", __PRETTY_FUNCTION__, fd);

	io::nonclosable_file_descriptor file(fd);
	file.seek(m_fh.index, ios_base::beg);

	io::filtering_istream in;
	m_fh.type.push(in);
	in.push(file);

	portable_binary_iarchive pba(in);
	pba >> lm;

	// Optimization on size. Overwrite the index during
	// next write if the index was stared on the end of the file.

	if (file.seek(0, ios_base::cur) == file.seek(0, ios_base::end))
		m_RawFileSize = m_fh.index;
}

void Compress::store(const FileHeader& fh, int fd)
{
	rDebug("%s: fd: %d", __PRETTY_FUNCTION__, fd);

	io::nonclosable_file_descriptor file(fd);
	file.seek(0, ios_base::beg);

	io::filtering_ostream out;
	out.push(file);

	portable_binary_oarchive pba(out);
	pba << fh;
}

void Compress::store(const LayerMap& lm, int fd, off_t offset, const CompressionType& type)
{
	rDebug("%s: fd: %d", __PRETTY_FUNCTION__, fd);

	io::nonclosable_file_descriptor file(fd);
	file.seek(offset, ios_base::beg);

	io::filtering_ostream out;
	type.push(out);
	out.push(file);

	portable_binary_oarchive pba(out);
	pba << lm;
}

int Compress::store(int fd)
{
	try {
		FileRememberTimes frt(fd);

		// Append new index to the end of the file.
		//
		store(m_lm, fd, m_RawFileSize, m_fh.type);
		m_fh.index = m_RawFileSize;
		store(m_fh, fd);
	}
	catch (...)
	{
		int tmp = errno;
		rError("%s: Failed to store FileHeader and/or LayerMap (%s)",
			__PRETTY_FUNCTION__, strerror(tmp));
		errno = tmp;
		return -1;
	}
	return 0;
}

extern unsigned int g_BufferedMemorySize;

// readFd - source file descriptor
// writeFd - destination file descriptor
// writeOffset - offset where start writing
// writeLm - store new Blocks there

off_t Compress::copy(int readFd, off_t writeOffset, int writeFd, LayerMap& writeLm)
{
	rDebug("copy, writeOffset: 0x%x", writeOffset);

	boost::scoped_array<char> buf(new char[g_BufferedMemorySize]);

	ssize_t bytes;

	// Start reading from the begining of the file.

	off_t readOffset = 0;

	while ((bytes = readCompressed(buf.get(), g_BufferedMemorySize, readOffset, readFd)) > 0)
	{
		rDebug("copy, readOffset: 0x%x, bytes: 0x%x", readOffset, bytes);

		writeOffset = writeCompressed(writeLm, readOffset, writeOffset, buf.get(), bytes, writeFd);
		readOffset += bytes;
	}
	return writeOffset;
}



void Compress::DefragmentFast()
{
//	return;
//	FileRememberTimes frt(m_fd);

	rDebug("%s", __PRETTY_FUNCTION__);

	struct stat st;
	struct timeval m_times[2];

	::fstat(m_fd, &st);
	m_times[0].tv_sec = st.st_atime;
	m_times[0].tv_usec = 0;
	m_times[1].tv_sec = st.st_mtime;
	m_times[1].tv_usec = 0;

	// Prepare a temporary file.

	char tmp_name[] = "./XXXXXX";

	int tmp_fd = mkstemp(tmp_name);
	if (tmp_fd < 0)
	{
		assert(errno != EINVAL);	// This would be a programmer error
		rError("%s: Cannot open temporary file.", __PRETTY_FUNCTION__);
		return;
	}

	::fchmod(tmp_fd, st.st_mode);
	::fchown(tmp_fd, st.st_uid, st.st_gid);
	::futimes(tmp_fd, m_times);

	// The inode number of the lower file has been changed
	// by rename, update the g_FileManager to reflect
	// that change. Without this the g_FileManager
	// would create an another File object for the
	// same file.

	::fstat(tmp_fd, &st);
	g_FileManager->Update(dynamic_cast<CFile*>(this), st.st_ino);

	if (::rename(tmp_name, m_name.c_str()) == -1)
	{
		rError("Cannot rename %s to %s", tmp_name, m_name.c_str());
		::close(tmp_fd);
		return;
	}

	// Reserve space for a FileHeader.

	off_t tmp_offset = FileHeader::MaxSize;

	// Temporary file prepared, now do the deframentation.

	LayerMap			tmp_lm;
	FileHeader			tmp_fh;

	tmp_offset = copy(m_fd, tmp_offset, tmp_fd, tmp_lm);

	tmp_fh.index = tmp_offset;
	tmp_fh.size = m_fh.size;

	m_RawFileSize = tmp_offset;

	store(tmp_lm, tmp_fd, tmp_fh.index, tmp_fh.type);
	store(tmp_fh, tmp_fd);

	// m_fd contains original file.
	// tmp_fd contains defragmented file.

	::futimes(tmp_fd, m_times);

	// m_fd contains only defragmented file.
	// tmp_fd no longer exists on the filesystem.
	// tpm_fh contains complete information.
	// tmp_lm contains complete information.

	m_lm.acquire(tmp_lm);
	m_fh.acquire(tmp_fh);

	::close(m_fd);
	m_fd = tmp_fd;
}

bool Compress::isCompressedOnlyWith(CompressionType& type)
{
	return m_lm.isCompressedOnlyWith(type);
}









#if 0
void Compress::DefragmentFast()
{
//	return;
//	FileRememberTimes frt(m_fd);

	rDebug("%s", __PRETTY_FUNCTION__);

	struct stat st;
	struct timeval m_times[2];
	::fstat(m_fd, &st);
	m_times[0].tv_sec = st.st_atime;
	m_times[0].tv_usec = 0;
	m_times[1].tv_sec = st.st_mtime;
	m_times[1].tv_usec = 0;

	int tmp_fd;
	off_t tmp_offset;

	// Prepare a temporary file.

	char tmp_name[] = "./XXXXXX";
rDebug("1");
	tmp_fd = mkstemp(tmp_name);
	if (tmp_fd < 0)
	{
		assert(errno != EINVAL);	// This would be a programmer error
		rError("%s: Cannot open temporary file.", __PRETTY_FUNCTION__);
		return;
	}
	if (::rename(tmp_name, m_name.c_str()) == -1)
	{
		rError("Cannot rename %s to %s", tmp_name, m_name.c_str());
		::close(tmp_fd);
		return;
	}
rDebug("2");

	// Reserve space for a FileHeader.

	tmp_offset = FileHeader::MaxSize;

	// Temporary file prepared, now do the deframentation.

	off_t to_write = 0;
	off_t offset = 0;
	off_t size = m_fh.size;

	LayerMap			tmp_lm;
	FileHeader			tmp_fh;

	io::nonclosable_file_descriptor	file(m_fd);
	io::nonclosable_file_descriptor	tmp_file(tmp_fd);
rDebug("3");

	tmp_offset = copy(m_fd, tmp_offset, tmp_fd, tmp_lm);
rDebug("4");
#if 0
	unsigned int level = 1;

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

		io::filtering_ostream out;

		if ((length == block->length) && (length == block->olength))
		{
			assert(offset == block->offset);

			// The whole block is in use, just copy raw block.

			io::filtering_istream in;
			in.push(file);

			buf = buf_write = new char[block->clength];

			file.seek(block->coffset, ios_base::beg);
			io::read(in, buf, block->clength);
			to_write = block->clength;

			block->level = level++;
		}
#if 1
		else
		{
			// Just a part of a block is in use, decompress whole block
			// and create a new block covering only used part.

			file.seek(block->coffset, ios_base::beg);

			io::filtering_istream in;
			block->type.push(in);
			in.push(file);

			block->type.push(out);

			buf = buf_write = new char[block->length];

			io::read(in, buf, block->length);
			to_write = length;

			buf_write += offset - block->offset;

			block->level = level++;
			block->offset = offset;
			block->length = length;
			block->olength = length;
		}
#endif
		block->coffset = tmp_offset;

		tmp_file.seek(tmp_offset, ios_base::beg);
		out.push(tmp_file);
		io::write(out, buf_write, to_write);
		out.reset();	// Flush the buffers

		tmp_offset = tmp_file.seek(0, ios_base::end);
		block->clength =  tmp_offset - block->coffset;

		delete[] buf;

		tmp_lm.Put(block, true);

		offset += length;
		size -= length;
	}
#endif
rDebug("5");

	tmp_fh.index = tmp_offset;
	tmp_fh.size = m_fh.size;

	rDebug("%s, m_IsCompressed: %d, m_fh.size: %llx", __PRETTY_FUNCTION__, m_IsCompressed, m_fh.size);

	store(tmp_lm, tmp_fd, tmp_fh.index, tmp_fh.type);
	store(tmp_fh, tmp_fd);
rDebug("6");

	m_RawFileSize = tmp_file.seek(0, ios_base::end);

	// m_fd contains original file.
	// tmp_fd contains defragmented file.
rDebug("7");

rDebug("8");

	::fchmod(tmp_fd, st.st_mode);
	::fchown(tmp_fd, st.st_uid, st.st_gid);
	::futimes(tmp_fd, m_times);
rDebug("9");

	// The inode number of the lower file has been changed
	// by rename, update the g_FileManager to reflect
	// that change. Without this the g_FileManager
	// would create an another File object for the
	// same file.

	::fstat(tmp_fd, &st);
	g_FileManager->Update(dynamic_cast<CFile*>(this), st.st_ino);

	// m_fd contains only defragmented file.
	// tmp_fd no longer exists on the filesystem.
	// tpm_fh contains complete information.
	// tmp_lm contains complete information.

	m_lm.acquire(tmp_lm);
	m_fh.acquire(tmp_fh);

	::close(m_fd);
	m_fd = tmp_fd;

	rDebug("%s, m_IsCompressed: %d, m_fh.size: %llx", __PRETTY_FUNCTION__, m_IsCompressed, m_fh.size);
}
#endif

