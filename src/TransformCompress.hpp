#ifndef TRANSFORMCOMPRESS_HPP
#define TRANSFORMCOMPRESS_HPP

#include "Transform.hpp"

#include <cassert>

/**
 * Abstract interface common for all compression transformations.
 */
class TransformCompress : public Transform
{
	/**
	 * Decompress data from buffer `from` of `from_size`. Store
	 * decompressed data to already created buffer
	 * at `to` of length `to_size`.
	 *
	 * Return
	 * 	-1 - Error
	 * 	 0 - Success
	 */
	virtual int decompress(const char *from, unsigned int  from_size,
	                             char *to,   unsigned int *to_size) = 0;

	/**
	 * Compress data from buffer `from` of `from_size` size. Function
	 * is supposed to create destination buffer and return
	 * pointer to it in `to` and set its size to the `to_size`.
	 *
	 * Return
	 * 	-1 - Error
	 * 	 0 - Success
	 */
	virtual int compress(const char *from, unsigned int from_size,
	                           char **to,  unsigned int *to_size) = 0;
public:
	virtual ~TransformCompress() { };

	Block *store(int fd, off_t offset, off_t coffset, const char *buf, size_t len);

	ssize_t restore(int fd, const Block *bl, off_t offset, char *buf, size_t len);
};

#endif

