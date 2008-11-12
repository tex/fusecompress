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

