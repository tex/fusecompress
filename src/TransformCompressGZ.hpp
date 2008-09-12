#ifndef TRANSFORMCOMPRESSGZ_HPP
#define TRANSFORMCOMPRESSGZ_HPP

#include "TransformCompress.hpp"

class TransformCompressGZ : public TransformCompress
{
	int decompress(const char *from, unsigned int  from_size,
	                     char *to,   unsigned int *to_size);

	int compress(const char *from, unsigned int from_size,
	                   char **to,  unsigned int *to_size);

	static const char m_name[3];
public:
	~TransformCompressGZ() { };

	const char *getName() { return m_name; }
};

#endif

