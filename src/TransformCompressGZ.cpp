#include "TransformCompressGZ.hpp"

#include <errno.h>
#include <rlog/rlog.h>
#include <stdlib.h>
#include <zlib.h>

#include <cassert>

const char TransformCompressGZ::m_name[3] = "gz";

int TransformCompressGZ::decompress(const char *from, unsigned int  from_size,
                                          char *to,   unsigned int *to_size)
{
	int	r;
	uLongf	gz_to_size;
	
	gz_to_size = *to_size;

	r = uncompress((Bytef*)to, &gz_to_size, (const Bytef*)from, from_size);
	if (r != Z_OK)
		return -1;

	*to_size = gz_to_size;

	return 0;
}

int TransformCompressGZ::compress(const char *from, unsigned int from_size,
                                        char **to,  unsigned int *to_size)
{
	int	 r;
	char	*gz_to;
	uLongf	 gz_to_size;

	gz_to_size = compressBound(from_size);
	gz_to = new (std::nothrow) char[gz_to_size];
	if (!gz_to)
	{
		rError("No memory to allocate block of %ld bytes",
				(long int) gz_to_size);

		errno = -ENOMEM;
		return -1;
	}

	r = compress2((Bytef*)gz_to, &gz_to_size,
	              (const Bytef*)from, from_size, Z_DEFAULT_COMPRESSION);
	if (r != Z_OK)
	{
		delete[] gz_to;

		rError("Failed compress2");

		errno = -EINVAL;
		return -1;
	}

	*to = gz_to;
	*to_size = gz_to_size;

	return 0;
}

