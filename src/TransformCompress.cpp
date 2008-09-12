#include <errno.h>
#include <cstring>

#include <rlog/rlog.h>

#include "TransformCompress.hpp"

extern size_t g_BufferedMemorySize;

Block *TransformCompress::store(int fd, off_t offset, off_t coffset, const char *buf, size_t len)
{
	char		*to = NULL;
	unsigned int	 to_len;
	ssize_t		 ssize;
	int		 r;

	r = compress(buf, len, &to, &to_len);
	if (r == -1)
		return NULL;
	assert (to);

	ssize = pwrite(fd, to, to_len, coffset);

	delete[] to;

	if (ssize != (ssize_t) to_len)
	{
		return NULL;
	}
	
	return new (std::nothrow) Block(offset, len, coffset, to_len);
}

ssize_t TransformCompress::restore(int fd, const Block *bl, off_t offset, char *buf, size_t len)
{
	char		*from = NULL;
	char		*to = NULL;
	unsigned int	 to_len;
	ssize_t		 ssize;

	assert (len <= bl->length);

	assert (offset >= bl->offset);
	assert (offset <= bl->offset + bl->length);

	from = new (std::nothrow) char[bl->clength];
	if (!from)
	{
		rError("No memory to allocate block of %d bytes",
				bl->clength);

		errno = -ENOMEM;
		goto err;
	}

	to_len = bl->olength;

	to = new (std::nothrow) char[to_len];
	if (!to)
	{
		rError("No memory to allocate block of %d bytes", to_len);

		errno = -ENOMEM;
		goto err;
	}

	ssize = pread(fd, from, bl->clength, bl->coffset);
	if (ssize == (ssize_t) bl->clength)
	{
		int ret = decompress(from, bl->clength, to, &to_len);
		if (ret == -1)
		{
			rWarning("File is corrupted");

			errno = -EINVAL;
			goto err;
		}

		memcpy(buf, to + (offset - bl->offset), len);
		ssize = len;
	}
	else
err:		ssize = -1;

	delete[] from;
	delete[] to;
	
	return ssize;
}

