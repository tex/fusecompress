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

#include "FileRememberTimes.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

FileRememberTimes::FileRememberTimes(int fd)
	: m_fd (fd)
{
	struct stat buf;

	fstat(fd, &buf);

	m_times[0].tv_sec = buf.st_atime;
	m_times[0].tv_nsec = buf.st_atim.tv_nsec;
	m_times[1].tv_sec = buf.st_mtime;
	m_times[1].tv_nsec = buf.st_mtim.tv_nsec;
}

FileRememberTimes::~FileRememberTimes()
{
	futimens(m_fd, m_times);
}

