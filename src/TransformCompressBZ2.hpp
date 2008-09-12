#ifndef TRANSFORMCOMPRESSBZ2_HPP
#define TRANSFORMCOMPRESSBZ2_HPP

#include "TransformCompress.hpp"

class TransformCompressBZ2 : public TransformCompress
{
	int decompress(const char *from, unsigned int  from_size,
	                     char *to,   unsigned int *to_size);

	int compress(const char *from, unsigned int from_size,
	                   char **to,  unsigned int *to_size);

	static const char m_name[4];
public:
	~TransformCompressBZ2() { };

	const char *getName() { return m_name; }
};

#endif

