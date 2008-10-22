#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <rlog/rlog.h>

class Mutex
{
	pthread_mutex_t	m_Mutex;

public:
	Mutex()
	{
		int r = pthread_mutex_init(&m_Mutex, NULL);
		if (r != 0)
		{
			rError("%s failed (%s)", __PRETTY_FUNCTION__, strerror(r));
			kill(0, SIGABRT);
		}
	}
	
	~Mutex()
	{
		int r = pthread_mutex_destroy(&m_Mutex);
		if (r != 0)
		{
			rError("%s failed (%s)", __PRETTY_FUNCTION__, strerror(r));
			kill(0, SIGABRT);
		}
	}

	void Lock(void)
	{
		int r = pthread_mutex_lock(&m_Mutex);
		if (r != 0)
		{
			rError("%s failed (%s)", __PRETTY_FUNCTION__, strerror(r));
			kill(0, SIGABRT);
		}
	}
	
	void Unlock(void)
	{
		int r = pthread_mutex_unlock(&m_Mutex);
		if (r != 0)
		{
			rError("%s failed (%s)", __PRETTY_FUNCTION__, strerror(r));
			kill(0, SIGABRT);
		}
	}
};

#endif

