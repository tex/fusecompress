#include "TransformTable.hpp"
#include <cstdlib>
#include <cstring>

#include <rlog/rlog.h>

TransformTable::TransformTable()
{
	m_table.reserve(5);

	// The order of Transformations is mandatory!! After
	// release to the public don't move any of these...
	//
	try
	{
		m_table.push_back(new TransformNONE());
		m_table.push_back(new TransformCompressBZ2());
		m_table.push_back(new TransformCompressGZ());
		m_table.push_back(new TransformCompressLZO());
		m_table.push_back(new TransformXOR());
	}
	catch (...)
	{
		rError("Out of memory!");
		abort();
	}

	// Set the TransformCompressLZO as the default
	// transformation...
	//
	m_default = m_table[3];
	m_default_index = 3;
}

TransformTable::~TransformTable()
{
	for (unsigned int i = 0; i < m_table.size(); i++)
	{
		delete m_table[i];
	}
}

bool TransformTable::setDefault(const string &name)
{
	for (unsigned int i = 0; i < m_table.size(); i++)
	{
		if (!strcmp(name.c_str(), m_table[i]->getName()))
		{
			m_default = m_table[i];
			m_default_index = i;
			return true;
		}
	}
	return false;
}

