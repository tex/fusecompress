#ifndef FILERAW_HPP
#define FILERAW_HPP

#include <sys/types.h>

class FileRaw
{
public:
	virtual ~FileRaw() { };

	virtual bool isTransformableToFileRawNormal() = 0;

	virtual int open(int fd) = 0;
	virtual int close() = 0;

	virtual ssize_t read(char *buf, size_t size, off_t offset) = 0;
	virtual ssize_t write(const char *buf, size_t size, off_t offset) = 0;

	virtual int truncate(const char *name, off_t size) = 0;
};

#endif

