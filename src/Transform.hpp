#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <sys/types.h>

#include "Block.hpp"

using namespace std;

class Transform
{
public:
	virtual ~Transform() { };
	
	/**
	 * Create Block with offset `offset`, length `len`, coffset `coffset`
	 * and clength as transformation needs. Input data starts on `buf` with
	 * length `len`. Store this Block to file descriptor `fd` and
	 * return pointer to this Block.
	 * 
	 * fd - use this file descriptor
	 * offset - write data to this offset in uncompressed stream
	 * coffset - write compressed data to this offset
	 * buf - store read data to this buffer
	 * len - lenght of the buffer
	 * 
	 * Return
	 * 	Pointer to Block which describe this Block.
	 */
	virtual Block *store(int fd, off_t offset, off_t coffset, const char *buf, size_t len) = 0;

	/**
	 * Read Block described by `bl` from file descriptor `fd` and copy data
	 * from offset `offset` with length `len` to the buffer `buf`.
	 * 
	 * fd - use this file descriptor
	 * bl - use this Block
	 * offset - read data from this offset in uncompressed stream
	 * buf - store read data to this buffer
	 * len - lenght of the buffer to fill
	 *
	 * Return
	 * 	Number of bytes read.
	 */
	virtual ssize_t restore(int fd, const Block *bl, off_t offset, char *buf, size_t len) = 0;

	/**
	 * Return name of the transformation.
	 */
	virtual const char *getName(void) = 0;
};

#endif

