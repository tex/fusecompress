#include <sys/time.h>

class FileRememberTimes
{
	int            m_fd;
	struct timeval m_times[2];
public:
	FileRememberTimes(int fd);
	~FileRememberTimes();
};

