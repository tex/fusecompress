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

#include "config.h"

#include <algorithm>
#include <functional>
#include <errno.h>
#include <sys/types.h>

#if defined(HAVE_ATTR_XATTR_H)
#  include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
#  include <sys/xattr.h>
#endif

#include "rlog/rlog.h"

#include <FileRememberXattrs.hpp>

void FileRememberXattrs::read(int fd_source)
{
	size_t list_size = 1024;
try_list_again:
	char* list = new char[list_size];
	int r = ::flistxattr(fd_source, list, list_size);
	if (-1 == r)
	{
		delete list;

		if (ERANGE == errno)
		{
			list_size *= 2;
			goto try_list_again;
		}
		return;
	}
	char *listp = list;
	while (r > 0)
	{
		int attr_size = 1024;
try_attr_again:
		char* attr = new char[attr_size];
		int rr = ::fgetxattr(fd_source, listp, attr, attr_size);
		if (-1 == rr)
		{
			delete attr;

			if (ERANGE == errno)
			{
				attr_size *= 2;
				goto try_attr_again;
			}
			delete list;
			return;
		}
		std::string key(listp);
		std::string value(attr, rr);
		m_xattrs[key] = value;

		listp += key.length() + 1;
		r -= key.length() + 1;
		delete attr;
	}
	delete list;
}

void FileRememberXattrs::write(int fd_dest)
{
	for (std::map<std::string, std::string>::const_iterator it = m_xattrs.begin(); it != m_xattrs.end(); ++it)
	{
		fsetxattr(fd_dest, it->first.c_str(), it->second.c_str(), it->second.size(), 0);
	}
}

