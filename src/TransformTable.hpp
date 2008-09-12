#ifndef TRANSFORMTABLE_HPP
#define TRANSFORMTABLE_HPP

#include <vector>
#include <string>

#include "TransformNONE.hpp"
#include "TransformXOR.hpp"
#include "TransformCompressLZO.hpp"
#include "TransformCompressGZ.hpp"
#include "TransformCompressBZ2.hpp"

using namespace std;

/**
 * This class holds all available transformations. Every
 * transformation must be created in the
 * TransformTable constructor.
 */
class TransformTable
{
	TransformTable(const TransformTable &);			// No copy constructor
	TransformTable& operator=(const TransformTable &);	// No assign operator

	vector<Transform *>	 m_table;
	Transform		*m_default;
	int			 m_default_index;
public:
	TransformTable();
	~TransformTable();

	inline Transform *getTransform(unsigned int index) const
	{
		// TODO: What to do if index is out of bounds? (File
		// got corrupted...)
		//
		assert (index >= 0);
		assert (index < m_table.size());

		return m_table[index];
	}
	
	inline Transform *getDefault() const
	{
		return m_default;
	}
	
	inline int getDefaultIndex() const
	{
		return m_default_index;
	}
	
	bool setDefault(const string &name);
};

#endif

