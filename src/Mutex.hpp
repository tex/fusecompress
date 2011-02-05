/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <signal.h>
#include <string.h>

#include "rlog/rlog.h"

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

