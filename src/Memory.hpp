#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "Compress.hpp"
#include "LinearMap.hpp"

#include <sys/types.h>

typedef Compress PARENT_MEMORY;
//typedef File PARENT_MEMORY;

/**
 * Class Memory represents memory backed file. It caches any writes to
 * the file and stores them in the memory. Memory blocks of some minimal
 * determined size are continually written to the disk.
 */
class Memory : public PARENT_MEMORY
{
protected:
	int write(bool force);
	int merge(const char *name);

	LinearMap	m_LinearMap;
	off_t		m_FileSize;
public:

	Memory(const struct stat *st);
	~Memory();

	int open(const char *name, int flags);

	int release(const char *name);

	int unlink(const char *name);

	int truncate(const char *name, off_t size);

	int getattr(const char *name, struct stat *st);

	ssize_t read(char *buf, size_t size, off_t offset);

	ssize_t write(const char *buf, size_t size, off_t offset);

	void Print(ostream &os) const;
};

ostream &operator<<(ostream &os, const Memory &rMemory);

#endif

