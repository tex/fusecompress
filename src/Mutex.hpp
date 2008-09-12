#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

class Mutex
{
	pthread_mutex_t	m_Mutex;

public:
	Mutex()
	{
		pthread_mutex_init(&m_Mutex, NULL);
	}
	
	~Mutex()
	{
		pthread_mutex_destroy(&m_Mutex);
	}

	void Lock(void)
	{
		pthread_mutex_lock(&m_Mutex);
	}
	
	void Unlock(void)
	{
		pthread_mutex_unlock(&m_Mutex);
	}
};

#endif

