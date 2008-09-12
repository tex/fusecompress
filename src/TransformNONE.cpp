#include "TransformNONE.hpp"

#include <errno.h>
#include <rlog/rlog.h>

#include <cassert>

const char TransformNONE::m_name[5] = "none";

Block *TransformNONE::store(int fd, off_t offset, off_t coffset, const char *buf, size_t len)
{
	ssize_t ssize;
	
	ssize = pwrite(fd, buf, len, coffset);
	if (ssize != (ssize_t) len)
	{
		rError("Failed pwrite");

		errno = -EINVAL;
		return NULL;
	}

	return new (std::nothrow) Block(offset, ssize, coffset, ssize);
}

ssize_t TransformNONE::restore(int fd, const Block *bl, off_t offset, char *buf, size_t len)
{
	assert (len <= bl->length);

	assert (offset >= bl->offset);
	assert (offset < bl->offset + bl->length);

	return pread(fd, buf, len, bl->coffset + (offset - bl->offset));
}

