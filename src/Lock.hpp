#ifndef LOCK_H
#define LOCK_H

#include "Mutex.hpp"

class Lock
{
	Mutex &m_rMutex;
public:
	Lock(Mutex &rMutex) :
		m_rMutex(rMutex)
	{
		m_rMutex.Lock();
	}
	
	~Lock()
	{
		m_rMutex.Unlock();
	}
};

#endif

