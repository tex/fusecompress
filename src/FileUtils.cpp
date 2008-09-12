#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cassert>

#include "FileUtils.hpp"

int FileUtils::force(const char *name, const struct stat &buf)
{
	int		r;
	int		fd;
	int		tmp_errno = 0;

	if (::chmod(name, buf.st_mode | 0600) == -1)
		return -1;

	fd = ::open(name, O_RDWR);
	if (fd == -1)
		tmp_errno = errno;

	r = ::chmod(name, buf.st_mode);
	assert (r != -1);

	if (fd == -1)
		errno = tmp_errno;
	
	return fd;
}

int FileUtils::open(const char *name)
{
	int 		fd;
	struct stat	buf;

	if (::stat(name, &buf) == -1)
		return -1;

	if (!S_ISREG(buf.st_mode))
		return -1;

	fd = ::open(name, O_RDWR);
	
	if ((fd == -1) && (errno == EACCES || errno == EPERM))
	{
		fd = force(name, buf);
	}

	return fd;
}


