#include "TransformCompressBZ2.hpp"

#include <errno.h>
#include <rlog/rlog.h>
#include <stdlib.h>
#include <bzlib.h>

#include <cassert>

const char TransformCompressBZ2::m_name[4] = "bz2";

int TransformCompressBZ2::decompress(const char *from, unsigned int  from_size,
                                           char *to,   unsigned int *to_size)
{
	int r;

	// const removed because compiler complains there
	// 
	r = BZ2_bzBuffToBuffDecompress(to, to_size, (char *) from, from_size, 0, 0);
	if (r != BZ_OK)
	{
		errno = -EINVAL;

		rError("Failed BZ2_bzBuffToBuffDecompress");
		return -1;
	}

	return 0;
}

int TransformCompressBZ2::compress(const char *from, unsigned int from_size,
                                         char **to,  unsigned int *to_size)
{
	int		 r;
	char		*bz2_to;
	unsigned int	 bz2_to_size;

	// Reserve about 1% and 600 bytes more (this is what BZ2
	// manual says).
	// 
	bz2_to_size = from_size + (from_size / 100) + 600;
	bz2_to = new (std::nothrow) char[bz2_to_size];
	if (!bz2_to)
	{
		rError("No memory to allocate block of %d bytes",
				bz2_to_size);

		errno = -ENOMEM;
		return -1;
	}

	r = BZ2_bzBuffToBuffCompress(bz2_to, &bz2_to_size,
	                             (char *) from, from_size, 6, 0, 0);
	if (r != BZ_OK)
	{
		delete[] bz2_to;

		rError("Failed BZ2_bzBuffToBuffCompress");

		errno = -EINVAL;
		return -1;
	}

	*to = bz2_to;
	*to_size = bz2_to_size;
	
	return 0;
}

