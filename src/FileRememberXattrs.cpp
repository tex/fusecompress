#include <FileRememberXattrs.hpp>

#include "config.h"

#include <rlog/rlog.h>
#include <algorithm>
#include <functional>
#include <errno.h>
#include <sys/types.h>
#if defined(HAVE_ATTR_XATTR_H)
#  include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
#  include <sys/xattr.h>
#endif

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

