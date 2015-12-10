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

