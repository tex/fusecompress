#include "TransformCompressLZO.hpp"

#include "minilzo/minilzo.h"

#include <errno.h>
#include <rlog/rlog.h>
#include <stdlib.h>

#include <cassert>

#define HEAP_ALLOC(var,size) \
	lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

const char TransformCompressLZO::m_name[4] = "lzo";

TransformCompressLZO::TransformCompressLZO()
{
	lzo_init();
}

int TransformCompressLZO::decompress(const char *from, unsigned int  from_size,
                                           char *to,   unsigned int *to_size)
{
	int		r;
	lzo_uint	lzo_to_size;

	lzo_to_size = *to_size;

	r = lzo1x_decompress_safe((lzo_bytep) from, from_size,
	                          (lzo_bytep) to, &lzo_to_size, NULL);
	if (r != LZO_E_OK)
		return -1;

	*to_size = lzo_to_size;

	return 0;
}

int TransformCompressLZO::compress(const char *from, unsigned int from_size,
                                         char **to,  unsigned int *to_size)
{
	int		 r;
	char		*lzo_to;
	lzo_uint	 lzo_to_size;

	// Work-memory needed for compression. Allocate memory in units
	// of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
	//
	HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

	lzo_to_size = from_size + from_size / 16 + 64 + 3;
	lzo_to = new (std::nothrow) char[lzo_to_size];
	if (!lzo_to)
	{
		rError("No memory to allocate block of %u bytes",
				static_cast<unsigned int> (lzo_to_size));

		errno = -ENOMEM;
		return -1;
	}

	r = lzo1x_1_compress((lzo_bytep) from, from_size,
	                     (lzo_bytep) lzo_to, &lzo_to_size, wrkmem);
	if (r != LZO_E_OK)
	{
		delete[] lzo_to;

		rError("Failed lzo1x_1_compress");

		errno = -EINVAL;
		return -1;
	}

	*to = lzo_to;
	*to_size = lzo_to_size;

	return 0;
}

