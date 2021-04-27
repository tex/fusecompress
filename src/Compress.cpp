/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    FuseCompress is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FuseCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/version.hpp>
#if BOOST_VERSION >= 105600
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/io/ios_state.hpp>

#include <boost/scoped_array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bytescounter.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/nonclosable_file_descriptor.hpp>
#include <boost/iostreams/slice.hpp>

#include <boost/archive/portable_iarchive.hpp>
#include <boost/archive/portable_oarchive.hpp>

#include "config.h"
#include "rlog/rlog.h"
#include "assert.h"

#include "Compress.hpp"
#include "FileUtils.hpp"
#include "FileRememberTimes.hpp"
#include "FileRememberXattrs.hpp"
#include "FileManager.hpp"

#include "CompressedMagic.hpp"

namespace io = boost::iostreams;
namespace se = boost::serialization;

extern CompressedMagic	 g_CompressedMagic;
extern CompressionType	 g_CompressionType;
extern FileManager	*g_FileManager;

std::ostream &operator<<(std::ostream &os, const Compress &rC)
{
	boost::io::ios_flags_saver ifs(os);

	os << std::hex;
	os << "--- m_fh.size: 0x" << rC.m_fh.size << std::endl;
	os << "m_LayerMap: " << std::endl << rC.m_lm << std::endl;
	os << "---" << std::endl;
	return os;
}

Compress::Compress(const struct stat *st, const char *name) :
	Parent (st, name)
{
	if (st->st_size == 0)
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
	else if (st->st_size < FileHeader::MinSize)
	{
		// Nonempty file with length smaller than minimal length of
		// the FileHeader has to be uncompressed.

		m_IsCompressed = false;
	}
	else
	{
		// This is a constructor, no one could
		// call the open() function yet.

		assert(m_fd == -1);

		try
		{
			restoreFileHeader(name);
			m_IsCompressed = m_fh.isValid();
		}
		catch (...)
		{
			m_IsCompressed = false;
		}
		if (m_IsCompressed)
		{
			m_RawFileSize = (m_fh.index == 0) ?
				      FileHeader::MaxSize : st->st_size;
		}
	}

	if (m_IsCompressed)
	{
		rDebug("C (%s), raw/user 0x%lx/0x%lx bytes",
				name, (long int) m_RawFileSize, (long int) m_fh.size);
	}
	else
	{
		rDebug("N (%s), 0x%lx bytes", name, (long int) st->st_size);
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

	if (m_IsCompressed)
	{
		m_lm.Truncate(0);	// Free allocated memory
		m_RawFileSize = 0;
	}

	return Parent::unlink(name);
}

int Compress::truncate(const char *name, off_t size)
{
	rDebug("%s name: %s, m_IsCompressed: %d, size: %lx",
	        __FUNCTION__, name, m_IsCompressed, (long int) size);

	if (!m_IsCompressed)
	{
		return ::truncate(name, size);
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

	// Truncate to zero lenght is the only what
	// we can implement easily for compressed file.
	//
	if (size == 0)
	{
		r = ::ftruncate(m_fd, FileHeader::MaxSize);
		if (r == -1)
			rWarning("Compress::truncate('%s', %ld) failed!",
			          name, (long int) size);

		m_RawFileSize = FileHeader::MaxSize;

		m_fh.size = size;
		m_lm.Truncate(size);
	}
	else
	{
		bool defragment = false;

		// Defragment the file only if result file size is
		// smaller then current file size.

		if (m_fh.size > size)
			defragment = true;

		m_fh.size = size;
		m_lm.Truncate(size);

		if (defragment)
			DefragmentFast();
	}

	r = store();

	if (openedHere)
	{
		release(name);
	}

	return r;
}

int Compress::getattr(const char *name, struct stat *st)
{
	int r;

	rDebug("%s name: %s, m_IsCompressed: %d, m_fh.size: 0x%lx",
	        __FUNCTION__, name, m_IsCompressed, (long int) m_fh.size);

	r = Parent::getattr(name, st);

	if (m_IsCompressed)
	{
		assert(m_fh.isValid() == true);

		st->st_size = m_fh.size;
	}

	return r;
}

int Compress::open(const char *name, int flags)
{
	assert(m_name == name);

	int r;

	r = Parent::open(name, flags);

	if ((m_refs == 1) && m_IsCompressed && (m_fh.index != 0))
	{
		try {
			restoreLayerMap();
		}
		catch (exception& e)
		{
			rError("%s: Failed to restore LayerMap of file '%s', exception: %s",
				__PRETTY_FUNCTION__, name, e.what());

			// Failed to restore LayerMap althrought it should be present. Mark the file
			// as not compressed to pass following release() correctly.

			m_IsCompressed = false;
			release(name);

			errno = EIO;
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
		store();
		m_lm.Truncate(0);
	}

	int r = Parent::release(name);

	rDebug("Compress::release m_refs: %d, m_IsCompressed: %d", m_refs, m_IsCompressed);

	return r;
}

off_t Compress::writeCompressed(LayerMap& lm, off_t offset, off_t coffset, const char *buf, size_t size, int fd, off_t rawFileSize)
{
	assert(coffset >= FileHeader::MaxSize);

	rDebug("offset: 0x%lx, coffset: 0x%lx, size: 0x%lx",
	       (long int) offset, (long int) coffset, (long int) size);

	Block *bl = NULL;

	try {
		// Append a new Block to the file.

		bl = new Block(g_CompressionType);

		bl->offset = offset;
		bl->coffset = coffset;
		bl->length = size;
		bl->olength = size;

		// Truncate the file to m_RawFileSize.
		//
		// This efectively removes layer map from the file, so if
		// anything wrong happens until store() is called we lost the
		// file!
		//
		// We have to do that until I find a way how to get length of
		// the compressed block that is written by io::write()...
		//
		// OK, I now know how to get length of the compressed block,
		// but I don't know anymore how to use it to avoid the truncation.

		assert(bl->coffset == rawFileSize);
		::ftruncate(fd, bl->coffset);

		// Compress and write block to the file.

		io::nonclosable_file_descriptor file(fd);
		file.seek(bl->coffset, ios_base::beg);
		io::bytescounter length;
		{
			io::filtering_ostream out;

			bl->type.push(out);
			out.push(boost::ref(length));
			out.push(file);

			io::write(out, buf, bl->length);

			// Destroying the object 'out' causes all filters to flush.
		}

		bl->clength = length.bytes();
		coffset = bl->coffset + bl->clength;
	}
	catch (exception& e)
	{
		rError("%s: Failed to add a new Block to the file, offset: %lx, coffset: %lx, exception: %s",
			__PRETTY_FUNCTION__, bl->offset, bl->coffset, e.what());

		delete bl;
		return -1;
	}
	
	assert(bl != NULL);
	lm.Put(bl);

	rDebug("length: 0x%lx", (long int) coffset);

	return coffset;
}

ssize_t Compress::write(const char *buf, size_t size, off_t offset)
{
	// Spurious call to write when file has not been opened
	// happened during testing...

	if (m_fd == -1)
	{
		rWarning("Compress::write Spurious call detected!");

		errno = -EBADF;
		return -1;
	}
	assert (m_fd != -1);

	rDebug("Compress::write size: 0x%lx, offset: 0x%lx",
	       (long int) size, (long int) offset);

	// We have an opportunity to decide whether we really
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
		return pwrite(m_fd, buf, size, offset);
	}
	else
	{
		// If we write data containing only zeros to the end of the file,
		// we can just increase size of the file. No need to really
		// compress and write buffer of zeros...

		if ((m_fh.size == offset) && FileUtils::isZeroOnly(buf, size))
		{
			assert(size > 0);
			m_fh.size = offset + size;
		}
		else
		{
			off_t rawFileSize = writeCompressed(m_lm, offset, m_RawFileSize, buf, size, m_fd, m_RawFileSize);
			if (rawFileSize == -1)
				return -1;
			m_RawFileSize = rawFileSize;

			assert(size > 0);
			m_fh.size = max(m_fh.size, (off_t) (offset + size));

			// Defragment the file only if raw file size if bigger than 4096 bytes
			// and raw file size is about 20% bigger than it would be uncompressed.

			if (m_RawFileSize > 4096 && m_RawFileSize > m_fh.size + ((m_fh.size * 2) / 10))
			{
				DefragmentFast();
			}
		}

		return size;
	}
}

/**
 * size - total number of bytes we want to read
 * len - number of bytes we can read from the specified block
 */
off_t Compress::readBlock(int fd, const Block& block, off_t size, off_t len, off_t offset, char *buf) const
{
	off_t r;

	io::nonclosable_file_descriptor file(fd);
	file.seek(block.coffset, ios_base::beg);

	io::filtering_istream in;
	block.type.push(in);
	in.push(io::slice(file, 0 /* already seeked and no seek after */, block.clength));

	boost::scoped_array<char> buf_tmp(new char[block.length]);

	// Optimization: read only as much bytes as necessary.

	r = min((off_t)(size), len);

	off_t not_needed = offset - block.offset;
	off_t must_read = not_needed + r;
	assert(block.length >= 0);
	assert(must_read <= (off_t) block.length);

	io::read(in, buf_tmp.get(), must_read);
	memcpy(buf, buf_tmp.get() + not_needed, r);

	return r;
}

/* m_fh.size, m_lm */
ssize_t Compress::readCompressed(char *buf, size_t size, off_t offset, int fd) const
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
				r = readBlock(fd, block, size, len, offset, buf);
			}
			catch (exception& e)
			{
				rError("%s: Block read failed: offset:%lx, coffset:%lx, length: %lx, clength: %lx, exception: %s",
					__PRETTY_FUNCTION__, (long int) block.offset, (long int) block.coffset,
				        (long int) block.length, (long int) block.clength, e.what());

				errno = EIO;
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

ssize_t Compress::read(char *buf, size_t size, off_t offset) const
{
	assert (m_fd != -1);
	assert (size >= 0);

	rDebug("Compress::read size: 0x%lx, offset: 0x%lx",
	       (long int) size, (long int) offset);

	if (m_IsCompressed == false)
	{
		return pread(m_fd, buf, size, offset);
	}
	else
	{
		return readCompressed(buf, size, offset, m_fd);
	}
}

void Compress::restoreFileHeader(const char *name)
{
	ifstream file(name);
	io::filtering_istream in;
	in.push(file);
	eos::portable_iarchive pba(in);
	pba >> m_fh;
}

/* m_fh must be correct. m_length may be changed. */
void Compress::restoreLayerMap()
{
	rDebug("%s: fd: %d", __PRETTY_FUNCTION__, m_fd);

	io::nonclosable_file_descriptor file(m_fd);
	file.seek(m_fh.index, ios_base::beg);

	io::filtering_istream in;
	m_fh.type.push(in);
	in.push(file);

	eos::portable_iarchive pba(in);
	pba >> m_lm;

	// Optimization on size. Overwrite the index during
	// next write if the index was stared on the end of the file.

	if (file.seek(0, ios_base::cur) == file.seek(0, ios_base::end))
		m_RawFileSize = m_fh.index;
}

void Compress::storeFileHeader() const
{
	rDebug("%s: m_fd: %d", __PRETTY_FUNCTION__, m_fd);

	io::nonclosable_file_descriptor file(m_fd);
	file.seek(0, ios_base::beg);

	io::filtering_ostream out;
	out.push(file);

	eos::portable_oarchive pba(out);
	pba << m_fh;
}

void Compress::storeLayerMap()
{
	rDebug("%s: m_fd: %d", __PRETTY_FUNCTION__, m_fd);

	// Don't store LayerMap if it has not been modified
	// since begining (open).

	if (!m_lm.isModified())
		return;

	io::nonclosable_file_descriptor file(m_fd);
	file.seek(m_RawFileSize, ios_base::beg);

	io::filtering_ostream out;
	m_fh.type.push(out);
	out.push(file);

	eos::portable_oarchive pba(out);
	pba << m_lm;

	// Set the file header's index to the current offset
	// where the index was saved.

	m_fh.index = m_RawFileSize;
}

int Compress::store()
{
	rDebug("%s", __PRETTY_FUNCTION__);

	try {
		FileRememberTimes frt(m_fd);

		// Append new index to the end of the file.
		//
		storeLayerMap();
		storeFileHeader();
	}
	catch (exception& e)
	{
		rError("%s: Failed to store FileHeader and/or LayerMap, exception: %s",
			__PRETTY_FUNCTION__, e.what());

		errno = EIO;
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
	boost::scoped_array<char> buf(new char[g_BufferedMemorySize]);

	ssize_t bytes;

	// Start reading from the begining of the file.

	off_t readOffset = 0;

	while ((bytes = readCompressed(buf.get(), g_BufferedMemorySize, readOffset, readFd)) > 0)
	{
		writeOffset = writeCompressed(writeLm, readOffset, writeOffset, buf.get(), bytes, writeFd, writeOffset);
		if (writeOffset == -1)
			return -1;
		readOffset += bytes;
	}
	return writeOffset;
}

off_t Compress::cleverCopy(int readFd, off_t writeOffset, int writeFd, LayerMap& writeLm)
{
	off_t offset = 0;
	off_t size = m_fh.size;

	Block	 block;
	off_t	 len;

	while (size > 0)
	{
		if (!m_lm.Get(offset, block, len))
		{
			// Block not found. There also is no block on a upper
			// offset.
			//
			break;
		}

		if (len)
		{
			// Block covers the offset, we can read len bytes
			// from it's de-compressed stream...

			try {
				boost::scoped_array<char> buf(new char[block.length]);

				// Read old block (or part of it we need)...

				off_t r = readBlock(readFd, block, size, len, offset, buf.get());

				// Write new block...

				writeOffset = writeCompressed(writeLm, offset, writeOffset, buf.get(), r, writeFd, writeOffset);
				if (writeOffset == -1)
					return -1;

				offset += r;
				size -= r;
			}
			catch (exception& e)
			{
				rError("%s: Block read failed: offset:%lx, coffset:%lx, length: %lx, clength: %lx, exception: %s",
					__PRETTY_FUNCTION__, (long int) block.offset, (long int) block.coffset,
				        (long int) block.length, (long int) block.clength, e.what());

				return -1;
			}
		}
		else
		{
			off_t r;

			// Block doesn't exists on the offset, but there is
			// a Block on the bigger offset.

			r = min(block.offset - offset, (off_t) (size));

			offset += r;
			size -= r;
		}
	}

	return writeOffset;
}

void Compress::DefragmentFast()
{
	rDebug("%s", __PRETTY_FUNCTION__);

	struct stat st;
	struct timespec m_times[2];

	::fstat(m_fd, &st);
	m_times[0].tv_sec = st.st_atime;
	m_times[0].tv_nsec = st.st_atim.tv_nsec;
	m_times[1].tv_sec = st.st_mtime;
	m_times[1].tv_nsec = st.st_mtim.tv_nsec;

	// Prepare a temporary file.

	char tmp_name[] = "./.fc.XXXXXX";

	int tmp_fd = mkstemp(tmp_name);
	if (tmp_fd < 0)
	{
		// EINVAL would be a programmer error.
		//
		assert(errno != EINVAL);

		rError("%s: Temporary file creation failed with errno: %d", __PRETTY_FUNCTION__, errno);
		return;
	}

	// Reserve space for a FileHeader.

	off_t tmp_offset = FileHeader::MaxSize;

	// Temporary file prepared, now do the deframentation.

	LayerMap tmp_lm;

	tmp_offset = cleverCopy(m_fd, tmp_offset, tmp_fd, tmp_lm);
//	tmp_offset = copy(m_fd, tmp_offset, tmp_fd, tmp_lm);

	if (tmp_offset == -1)
	{
		::unlink(tmp_name);
		::close(tmp_fd);
		return;
	}

	// Transfer file attributes like mode, owner, times and
	// extended attributes.

	::fchmod(tmp_fd, st.st_mode);
	::fchown(tmp_fd, st.st_uid, st.st_gid);
	::futimens(tmp_fd, m_times);

	FileRememberXattrs xattrs;
	xattrs.read(m_fd);
	xattrs.write(tmp_fd);

	::close(m_fd);
	m_fd = tmp_fd;

	// Store new file header and layer map to the new file.
	// m_fd contains file descriptor of the new file and
	// file header is the same except m_fh.index but
	// the index will be updated in the store() function.
	// Update m_RawFileSize to tell store() where save the
	// index and set m_lm to layer map of the new file.

	m_RawFileSize = tmp_offset;

	// Set index to zero (no index). Index will be set to
	// correct value in store according to m_RawFileSize and
	// existence of modified layer map.

	m_fh.index = 0;

	m_lm = tmp_lm;

	store();

	// The inode number of the lower file will be changed
	// by rename so we have to update the g_FileManager to reflect
	// that change. Without this update the g_FileManager
	// would create an another File object for the same file.

	::fstat(tmp_fd, &st);

	g_FileManager->Lock();
	g_FileManager->UpdateUnlocked(dynamic_cast<CFile*>(this), st.st_ino);

	if (::rename(tmp_name, m_name.c_str()) == -1)
	{
		rError("%s: Cannot rename '%s' to '%s'",
			__PRETTY_FUNCTION__, tmp_name, m_name.c_str());
	}
	g_FileManager->Unlock();
}

bool Compress::isCompressedOnlyWith(CompressionType& type)
{
	return m_lm.isCompressedOnlyWith(type);
}

