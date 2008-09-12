#include "TransformXOR.hpp"

#include <errno.h>
#include <rlog/rlog.h>

#include <cassert>

const char TransformXOR::m_name[4] = "xor";

Block *TransformXOR::store(int fd, off_t offset, off_t coffset, const char *buf, size_t len)
{
	char	*b;
	ssize_t  ssize;
	
	// Example: Simply encode buffer...
	//
	b = new (std::nothrow) char[len];
	if (!b)
	{
		rError("No memory to allocate block of %d bytes",
				len);

		errno = -ENOMEM;
		return NULL;
	}

	for (size_t i = 0; i < len; i++)
		b[i] = buf[i] ^ 2;

	ssize = pwrite(fd, buf, len, coffset);

	delete[] b;

	if (ssize != (ssize_t) len)
	{
		rError("Failed pwrite");

		errno = -EINVAL;
		return NULL;
	}

	return new (std::nothrow) Block(offset, ssize, coffset, ssize);
}

ssize_t TransformXOR::restore(int fd, const Block *bl, off_t offset, char *buf, size_t len)
{
	ssize_t ssize;

	assert (len <= bl->length);

	assert (offset >= bl->offset);
	assert (offset < bl->offset + bl->length);

	ssize = pread(fd, buf, len, bl->coffset + (offset - bl->offset));
	if (ssize == -1)
		return -1;

	// Example: Simply decode data...
	//
	for (size_t i = 0; i < (size_t) ssize; i++)
		buf[i] = buf[i] ^ 2;

	return ssize;
}

