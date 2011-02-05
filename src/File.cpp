/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rlog/rlog.h"
#include "assert.h"

#include "File.hpp"
#include "FileUtils.hpp"

File::File(const struct stat *st, const char *name) :
	m_fd (-1),
	m_inode (st->st_ino),
	m_refs (0),
	m_name (name)
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

	m_name = name;

	if (m_fd != -1)
		r = ::fstat(m_fd, st);
	else
		r = ::lstat(name, st);

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

	if (m_fd != -1)
		r = ::ftruncate(m_fd, size);
	else
		r = ::truncate(name, size);

	return r;
}

int File::utimens(const char *name, const struct timespec tv[2])
{
	int r;

	r = ::utimensat(AT_FDCWD, name, tv, AT_SYMLINK_NOFOLLOW);

	return r;
}

ssize_t File::read(char *buf, size_t size, off_t offset) const
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

	rDebug("File::open file '%s', inode %ld, m_refs: %d",
		name, (long int) m_inode, m_refs);

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

	rDebug("File::release m_refs: %d", m_refs);

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

