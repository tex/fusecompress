#include <iostream>

#include <boost/serialization/access.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/version.hpp>

#include "Block.hpp"

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
	typedef std::multiset<Block *, ltComp> con_t;

	con_t        m_Map;
	unsigned int m_MaxLevel;
	unsigned int m_MaxLength;

	void next(con_t::iterator &it);
	unsigned int length(con_t::iterator &it, off_t offset);
	void find(off_t offset, con_t::iterator &it);

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned version)
	{
		ar & m_MaxLevel;
		ar & m_MaxLength;
		ar & m_Map;
	}
public:
	LayerMap() :
		m_MaxLevel(1),
		m_MaxLength(0)
	{}

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

	void Print(std::ostream &os) const;
};

BOOST_CLASS_VERSION(LayerMap, 0)

// Don't track instances of this class. Users creates
// them on the stack.

BOOST_CLASS_TRACKING(LayerMap, boost::serialization::track_never)

std::ostream &operator<<(std::ostream &os, const LayerMap &rLm);

