#ifndef COMPRESS_HPP
#define COMPRESS_HPP

#include "File.hpp"
#include "FileRaw.hpp"
#include "FileHeader.hpp"

#include <sys/types.h>

typedef File PARENT_COMPRESS;

/**
 */
class Compress : public PARENT_COMPRESS
{
private:
	void createFileRaw(const char *name);

	void restore(FileHeader& fh, const char *name);
	void restore(FileHeader& fh, int fd);

	// Pointer to FileRaw class instance
	// (can be FileRawNormal or FileRawCompressed).
	//
	FileRaw	*m_FileRaw;

	// Length of the lower file
	// (not as seen by the user via fuse mount point).
	//
	off_t	 m_RawFileSize;
public:
	Compress(const struct stat *st);
	~Compress();

	int open(const char *name, int flags);

	int release(const char *name);

	int unlink(const char *name);

	int truncate(const char *name, off_t size);

	int getattr(const char *name, struct stat *st);

	ssize_t read(char *buf, size_t size, off_t offset);

	ssize_t write(const char *buf, size_t size, off_t offset);
};

#endif

