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
private:
	typedef PARENT_MEMORY Parent;

	int write(bool force);
	int merge(const char *name);
	ssize_t readFullParent(char * &buf, size_t &len, off_t &offset) const;
	ssize_t readParent(char * &buf, size_t &len, off_t &offset, off_t block_offset) const;
	void copyFromBlock(char * &buf, size_t &len, off_t &offset, char *block, size_t block_size, off_t block_offset) const;

	LinearMap	m_LinearMap;

	// Length of the file as seen by the user via fuse mount point.
	//
	off_t		m_FileSize;
	bool		m_FileSizeSet;
	struct timespec m_Time[2];
	bool		m_TimeSet;
public:

	Memory(const struct stat *st, const char *name);
	~Memory();

	int open(const char *name, int flags);

	int release(const char *name);

	int unlink(const char *name);

	int truncate(const char *name, off_t size);

	int getattr(const char *name, struct stat *st);

	ssize_t read(char *buf, size_t size, off_t offset) const;

	ssize_t write(const char *buf, size_t size, off_t offset);

	int utimens(const char *name, const struct timespec tv[2]);

	friend ostream &operator<<(ostream &os, const Memory &rMemory);
};

#endif

