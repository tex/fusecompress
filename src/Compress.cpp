#include <algorithm>
#include <iostream>
#include <sstream>
#include <rlog/rlog.h>

#include <assert.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"

#if defined(HAVE_ATTR_XATTR_H)
#  include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
#  include <sys/xattr.h>
#endif

#include "Compress.hpp"
#include "FileRawCompressed.hpp"
#include "FileRawNormal.hpp"

#include "TransformTable.hpp"
#include "CompressedMagic.hpp"

extern TransformTable	g_TransformTable;
extern CompressedMagic	g_CompressedMagic;

void Compress::createFileRaw(const char *name)
{
	bool		fl = true;
	FileHeader	fh;

	if (m_FileSize != 0)
	{
		fl = fh.restore(name);
	}
	else
	{
		// Create an compressed file as default. It will
		// be converted to normal file in write() function
		// if the content is uncompressable.
		//
		fh.type = g_TransformTable.getDefaultIndex();
		fl = true;
	}

	if (fl)
	{
		rDebug("C (%s), real/orig %ld/%ld bytes",
				name, (long int) m_FileSize, (long int) fh.size);

		m_FileRaw = new (std::nothrow) FileRawCompressed(fh, m_FileSize);
	}
	else
	{
		rDebug("N (%s), %ld bytes", name, m_FileSize);

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

	if (fh.restore(name))
	{
		st->st_size = fh.size;
		return r;
	}

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
		m_FileRaw->open(m_fd);
	}

	return r;
}

int Compress::release(const char *name)
{
	if ((m_refs == 1) && (m_FileRaw))
	{
		m_FileRaw->close();
	}
	
	return File::release(name);
}

ssize_t Compress::write(const char *buf, size_t size, off_t offset)
{
	assert (m_FileRaw);
	
	rDebug("Compress::write size: %d, offset: %lld", (unsigned int) size,
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

	rDebug("Compress::read size: %d, offset: %lld", (unsigned int) size,
			(long long int) offset);

	return m_FileRaw->read(buf, size, offset);
}

