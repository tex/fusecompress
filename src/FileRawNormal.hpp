#ifndef FILERAWNORMAL_HPP
#define FILERAWNORMAL_HPP

#include "FileRaw.hpp"

class FileRawNormal : public FileRaw
{
private:
	int m_fd;
public:
	FileRawNormal();
	FileRawNormal(int fd);
	~FileRawNormal();

	// FileRawNormal is not transformable to FileRawNormal.
	//
	inline bool isTransformableToFileRawNormal() { return false; }
	
	int open(int fd);
	int close();

	ssize_t read(char *buf, size_t size, off_t offset);
	ssize_t write(const char *buf, size_t size, off_t offset);

	int truncate(const char *name, off_t size);
	void getattr(const char *name, struct stat *st);
};

#endif

