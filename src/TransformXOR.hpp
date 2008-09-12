#ifndef TRANSFORMXOR_HPP
#define TRANSFORMXOR_HPP

#include <sys/types.h>

#include "Transform.hpp"

class TransformXOR : public Transform
{
	static const char m_name[4];
public:
	~TransformXOR() { };
	
	Block *store(int fd, off_t offset, off_t coffset, const char *buf, size_t len);

	ssize_t restore(int fd, const Block *bl, off_t offset, char *buf, size_t len);

	const char *getName() { return m_name; }
};

#endif

