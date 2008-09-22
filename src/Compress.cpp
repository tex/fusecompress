#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <rlog/rlog.h>

#include <assert.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

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
#include "FileRawCompressed.hpp"
#include "FileRawNormal.hpp"

#include "CompressedMagic.hpp"

using namespace std;
using namespace boost::iostreams;

extern CompressedMagic	 g_CompressedMagic;

void Compress::restore(FileHeader& fh, const char *name)
{
	if (m_fd != -1)
	{
		filtering_istream in;
		nonclosable_file_descriptor filefd(m_fd);
		rDebug("Using nonclosable_file_descriptor");
		in.push(filefd);
		portable_binary_iarchive pba(in);
		pba >> fh;
	}
	else
	{
		filtering_istream in;
		ifstream file(name);
		rDebug("Using file name");
		in.push(file);
		portable_binary_iarchive pba(in);
		pba >> fh;
	}
}

void Compress::createFileRaw(const char *name)
{
	bool		fl = true;
	FileHeader	fh;

	if (m_FileSize >= FileHeader::MaxSize)
	{
		restore(fh, name);
		fl = fh.isValid();
	}

	if (fl)
	{
		rDebug("C (%s), real/orig 0x%lx/0x%lx bytes",
				name, (long int) m_FileSize, (long int) fh.size);

		m_FileRaw = new (std::nothrow) FileRawCompressed(fh, m_FileSize);
	}
	else
	{
		rDebug("N (%s), 0x%lx bytes", name, m_FileSize);

		m_FileRaw = new (std::nothrow) FileRawNormal();
	}

	if (!m_FileRaw)
	{
		rError("No memory to allocate instance of FileRaw interface");
		abort();
	}
}

Compress::Compress(const struct stat *st) :
	File (st),
	m_FileRaw (NULL),
	m_FileSize (st->st_size)
{
}

Compress::~Compress()
{
	delete m_FileRaw;
}

int Compress::unlink(const char *name)
{
	rDebug("%s", __FUNCTION__);

	delete m_FileRaw;
	
	m_FileSize = 0;
	m_FileRaw = NULL;

	return PARENT_COMPRESS::unlink(name);
}

int Compress::truncate(const char *name, off_t size)
{
	if (!m_FileRaw)
		createFileRaw(name);
/*
	// PUT SOME HERUISTIC HERE TO DETERMINE WHEN TRANSFORM THE FILE
	// FROM COMPRESSED TO UNCOMPRESSED FORMAT

	if (size != 0)
	{
		// Ok, size if different than 0, compressed archive
		// would grow because we cannot truncate compressed archives.
		// So we decompress the file with the hope that we will be
		// able to compress it later.

		// CREATE A THREAD THAT WOULD COLLECT NAMES OF FILES THAT
		// HAVE BEEN DECOMPRESSED

		m_FileRaw = FileRawCompressed::TransformToFileRawNormal(m_FileRaw);
	}
*/
	return m_FileRaw->truncate(name, size);
}

int Compress::getattr(const char *name, struct stat *st)
{
	int        r;
	FileHeader fh;

	r = PARENT_COMPRESS::getattr(name, st);

	if (r != 0)
	{
		return r;
	}

	if (m_FileRaw)
	{
		m_FileRaw->getattr(name, st);
		return r;
	}

	if (st->st_size < FileHeader::MaxSize)
	{
		return r;
	}
try{
	restore(fh, name);

	if (fh.isValid())
	{
		st->st_size = fh.size;
		return r;
	}
} catch (...) {}
	return r;
}

int Compress::open(const char *name, int flags)
{
	int r;

	r = PARENT_COMPRESS::open(name, flags);

	if (!m_FileRaw)
		createFileRaw(name);

	if (m_refs == 1)
	{
		m_FileRaw->open(r);
	}

	rDebug("Compress::open m_refs: %d", m_refs);

	return r;
}

int Compress::release(const char *name)
{
	if ((m_refs == 1) && (m_FileRaw))
	{
		m_FileRaw->close();
	}

	int r = PARENT_COMPRESS::release(name);

	rDebug("Compress::release m_refs: %d", m_refs);

	return r;
}

ssize_t Compress::write(const char *buf, size_t size, off_t offset)
{
	assert (m_FileRaw);
	
	rDebug("Compress::write size: 0x%x, offset: 0x%llx", (unsigned int) size,
			(long long int) offset);

	// We have an oppourtunity to change the m_FileRaw from 
	// FileRawCompressed class into FileRawNormal for cases
	// where file name test was not enought to determine the real
	// type of data beeing stored...
	// 
	if ((offset == 0) && (m_FileSize == 0) &&
	    (m_FileRaw->isTransformableToFileRawNormal()) &&
	    (g_CompressedMagic.isNativelyCompressed(buf, size)))
	{
		m_FileRaw = FileRawCompressed::TransformToFileRawNormal(m_FileRaw);
	}

	ssize_t ret = m_FileRaw->write(buf, size, offset);
	if (ret > 0)
	{
		// Bufer successfuly written, update m_FileSize.
		// This may help to decrease the time spend in
		// the previous test (g_CompressedMagic is slow).
		// 
		m_FileSize = max(m_FileSize, offset + ret);
	}

	return ret;
}

ssize_t Compress::read(char *buf, size_t size, off_t offset)
{
	assert (m_FileRaw);

	rDebug("Compress::read size: 0x%x, offset: 0x%llx", (unsigned int) size,
			(long long int) offset);

	return m_FileRaw->read(buf, size, offset);
}

