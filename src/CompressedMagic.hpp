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

#ifndef __COMPRESSED_MAGIC__
#define __COMPRESSED_MAGIC__

#include <magic.h>
#include <strings.h>

#include <iostream>
#include <set>
#include <string>

#include "Mutex.hpp"

class CompressedMagic
{
	typedef std::set<std::string> con_t;
	con_t    m_table;
	magic_t  m_magic;
	Mutex    m_Mutex;

	void PopulateTable();
public:
	CompressedMagic();
	~CompressedMagic();

	bool isNativelyCompressed(const char *buf, int len);
	bool isNativelyCompressed(const char *name);

	void Remove(const std::string &rMime);
	void Add(const std::string &rMime);

	friend std::ostream &operator<<(std::ostream &os, const CompressedMagic &rObj);
};

#endif

