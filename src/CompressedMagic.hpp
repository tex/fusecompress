#ifndef __COMPRESSED_MAGIC__
#define __COMPRESSED_MAGIC__

#include <set>
#include <magic.h>
#include <strings.h>
#include <string>

// #include <iostream>

#include "Mutex.hpp"

using namespace std;

class CompressedMagic
{
	typedef set<string> con_t;
	con_t m_table;

	magic_t m_magic;

	Mutex m_Mutex;

	void PopulateTable();
public:
	CompressedMagic();
	~CompressedMagic();

	bool isNativelyCompressed(const char *buf, int len);
	bool isNativelyCompressed(const char *name);

	void Remove(const string &rMime);
	void Add(const string &rMime);

	void Print(ostream &os) const;
};

ostream &operator<<(ostream &os, const CompressedMagic &rObj);

#endif

