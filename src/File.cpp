#include <unistd.h>
#include <utime.h>
#include <errno.h>

#include <assert.h>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <rlog/rlog.h>

#include "File.hpp"
#include "FileUtils.hpp"

File::File(const struct stat *st) :
	m_fd (-1),
	m_inode (st->st_ino),
	m_refs (0)
{
}

File::~File()
{
	assert (m_refs == 0);
	assert (m_fd == -1);
}

int File::getattr(const char *name, struct stat *st)
{
	int r;

	m_FileName = name;

	if (m_fd != -1)
	{
		r = ::fstat(m_fd, st);
	}
	else
	{
		r = ::lstat(name, st);
	}

	return r;
}

int File::unlink(const char *name)
{
	int r;

	r = ::unlink(name);

	return r;
}

int File::truncate(const char *name, off_t size)
{
	int r;

	r = ::truncate(name, size);

	return r;
}

int File::utime(const char *name, struct utimbuf *buf)
{
	int r;

	r = ::utime(name, buf);

	return r;
}

ssize_t File::read(char *buf, size_t size, off_t offset)
{
	ssize_t r;
	
	assert(m_fd >= 0);

	r = ::pread(m_fd, buf, size, offset);

	return r;
}

ssize_t File::write(const char *buf, size_t size, off_t offset)
{
	ssize_t r;

	assert(m_fd >= 0);

	r = ::pwrite(m_fd, buf, size, offset);

	return r;
}

int File::open(const char *name, int flags)
{
	// Use only one file descriptor for one file determined
	// by inode number.
	//
	if (m_fd != -1)
	{
		++m_refs;
		return m_fd;
	}
	
	m_fd = FileUtils::open(name);
	
	if (m_fd != -1)
	{
		++m_refs;
	}

	rDebug("File::open file '%s', inode %ld",
		name, (long int) m_inode);

	return m_fd;
}

int File::flush(const char *name)
{
	return 0;
}

int File::release(const char *name)
{
	if (--m_refs == 0)
	{
		::close(m_fd);
		m_fd = -1;
	}
	assert (m_refs >= 0);

	return 0;
}

int File::fdatasync(const char *name)
{
	assert(m_fd >= 0);

	return ::fdatasync(m_fd);
}

int File::fsync(const char *name)
{
	assert(m_fd >= 0);

	return ::fsync(m_fd);
}

