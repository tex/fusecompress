#include "FileRememberTimes.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

FileRememberTimes::FileRememberTimes(int fd)
	: m_fd (fd)
{
	struct stat buf;

	fstat(fd, &buf);

	m_times[0].tv_sec = buf.st_atime;
	m_times[0].tv_nsec = buf.st_atim.tv_nsec;
	m_times[1].tv_sec = buf.st_mtime;
	m_times[1].tv_nsec = buf.st_mtim.tv_nsec;
}

FileRememberTimes::~FileRememberTimes()
{
	futimens(m_fd, m_times);
}

