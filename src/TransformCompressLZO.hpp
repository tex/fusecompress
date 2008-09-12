#ifndef TRANSFORMCOMPRESSLZO_HPP
#define TRANSFORMCOMPRESSLZO_HPP

#include "TransformCompress.hpp"

class TransformCompressLZO : public TransformCompress
{
	int decompress(const char *from, unsigned int  from_size,
	                     char *to,   unsigned int *to_size);

	int compress(const char *from, unsigned int  from_size,
	                   char **to,  unsigned int *to_size);

	static const char m_name[4];
public:
	TransformCompressLZO();
	~TransformCompressLZO() { };

	const char *getName() { return m_name; }
};

#endif

