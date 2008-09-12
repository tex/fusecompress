#include <cassert>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "FileRawNormal.hpp"

using namespace std;

FileRawNormal::FileRawNormal() :
	m_fd (-1)
{
}

FileRawNormal::FileRawNormal(int fd) :
	m_fd (fd)
{
}

FileRawNormal::~FileRawNormal()
{
	m_fd = -1;
}

int FileRawNormal::open(int fd)
{
	assert (m_fd == -1);
	
	m_fd = fd;

	return fd;
}

int FileRawNormal::close()
{
	if (m_fd == -1)
	{
		return 0;
	}
	
	m_fd = -1;

	return 0;
}

ssize_t FileRawNormal::read(char *buf, size_t size, off_t offset)
{
	assert (m_fd != -1);

	return pread(m_fd, buf, size, offset);
}

ssize_t FileRawNormal::write(const char *buf, size_t size, off_t offset)
{
	assert (m_fd != -1);

	return pwrite(m_fd, buf, size, offset);
}

int FileRawNormal::truncate(const char *name, off_t size)
{
	return ::truncate(name, size);
}

