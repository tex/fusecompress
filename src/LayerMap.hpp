#include <iostream>
#include <set>

#include "Block.hpp"
#include "Transform.hpp"

using namespace std;

class LayerMap
{
private:
	struct ltComp
	{
		// Required sorting order is as follows:
		//
		// (offset, level)
		// (  0,      2  )
		// (  0,      1  )
		// (  7,      3  )
		// (  9,      5  )
		// (  9,      4  )
		//
		bool operator()(const Block *pB1, const Block *pB2) const
		{
			if (pB1->offset < pB2->offset)
				return true;
			if (pB1->offset > pB2->offset)
				return false;
			return (pB1->level > pB2->level);
		}
	};
	typedef multiset<Block *, ltComp> con_t;

	con_t        m_Map;
	unsigned int m_MaxLevel;
	unsigned int m_MaxLength;

	void prev(con_t::iterator &it);
	void next(con_t::iterator &it);
	unsigned int length(con_t::iterator &it, off_t offset);
	void find(off_t offset, con_t::iterator &it);
	void recalcMaxLength();
//
	Transform *m_transform;

	int store(char *buf, size_t len) const;

	int restore(size_t count, const char *buf, size_t len);
//
public:
	LayerMap() :
		m_MaxLevel(1),
		m_MaxLength(0)
	{}

	LayerMap(Transform *transform) :
		m_MaxLevel(1),
		m_MaxLength(0),
		m_transform(transform)
	{}
//
	void set(Transform *transform)
	{
		m_transform = transform;
	}
	/*
	 * Object persistion, store object to file.
	 * 
	 * File format of the index on the disk is as follows:
	 * 	Block  - Block describing compressed buffer with index
	 * 	char[] - Buffer with compressed index
	 */
	off_t store(int fd, off_t to) const;

	/*
	 * Object persistion, restore object from file.
	 */
	off_t restore(int fd, off_t from);
//
	void Put(Block *pBl, bool bKeepLevel = false);

	/**
	 * Description:
	 *  Return iterator that points to a Block that covers the specified offset.
	 *  Caller may read up to length bytes from the Block.
	 *
	 * Input:
	 *  offset - wants to read from the specified offset
	 * Output:
	 *  it - return iterator that fits
	 *  length - number of bytes to read from the Block pointed to by the returned iterator
	 * Return:
	 *  bool - true if something found, false if found nothing
	 */
	bool Get(off_t offset, Block &rBlock, off_t &rLength);

	void Truncate(off_t length);
	void Print(ostream &os) const
	{
		con_t::const_iterator it;

		os << "-- m_MaxLevel: 0x" << hex << m_MaxLevel <<
		      " -------" << endl;

		for (it = m_Map.begin(); it != m_Map.end(); ++it)
		{
			os << **it << endl;
		}

		os << "---------";
	}
};


