#include "LayerMap.hpp"

#include <algorithm>
#include <cassert>

#include <boost/io/ios_state.hpp>
#include <rlog/rlog.h>

using namespace std;

std::ostream &operator<<(std::ostream &os, const LayerMap &rLm)
{
	rLm.Print(os);
	return os;
}

void LayerMap::Print(std::ostream &os) const
{
	boost::io::ios_flags_saver ifs(os);

	os << std::hex;
	os << "-- m_MaxLevel: 0x" << m_MaxLevel <<
	      ", m_MaxLength: 0x" << m_MaxLength <<
	      " -------" << std::endl;

	for (con_t::const_iterator it = m_Map.begin(); it != m_Map.end(); ++it)
	{
		os << **it << std::endl;
	}

	os << "---------";
}

void LayerMap::Put(Block *pBl, bool bKeepLevel)
{
//	cout << "Put Block: " << *pBl << " into following LayerMap:" << endl;
//	cout << *this << endl;

	assert(pBl->length > 0);

	// Preserve a level if already set.
	//
	if (!bKeepLevel)
		pBl->level = m_MaxLevel++;
	con_t::iterator it = m_Map.insert(pBl);
	if (m_MaxLength < pBl->length)
		m_MaxLength = pBl->length;

//	cout << "State after Put: " << endl << *this << endl;
}

void LayerMap::Truncate(off_t offset)
{
//	std::cout << "Truncate following LayerMap to 0x" << std::hex << length << std::endl;
//	std::cout << *this << std::endl;

	con_t::iterator it = m_Map.begin();

	while (it != m_Map.end())
	{
		if ((*it)->offset >= offset)
		{
			delete (*it);
			m_Map.erase(it++);
			continue;
		}
		if ((*it)->offset + (off_t) (*it)->length > offset)
		{
			(*it)->length = offset - (*it)->offset;
			assert((*it)->length > 0);
		}
		++it;
	}
//	std::cout << "State after Truncate: " << std::endl << *this << std::endl;
}

/* Find the biggest leveled Block with the higher offset and higher level than
 * current 'it' references to. */
void LayerMap::next(off_t offset, con_t::iterator &it)
{
	unsigned int level = (*it)->level;

	while (++it != m_Map.end())
	{
		if ((*it)->offset < offset)
			continue;
		if ((*it)->level > level)
			break;
	}
}

/* Returns number of bytes that can be read from the offset from the Block specified by 'it' iterator. */
unsigned int LayerMap::length(con_t::iterator &it, off_t offset)
{
	if ((*it)->offset > offset)
	{
		// Block starts after the offset, return 0 to
		// indicate that.
		//
		return 0;
	}

	con_t::iterator ni = it;

	next(offset, ni);

	if ((ni == m_Map.end()) ||
	    ((*it)->offset + (*it)->length <= (*ni)->offset))
	{
		// 'it' block is not limited with 'no' block.
		//
		return (*it)->length - (offset - (*it)->offset);
	}
	else
	{
		return ((*ni)->offset - (*it)->offset) - (offset - (*it)->offset);
	}
}

void LayerMap::find(off_t offset, con_t::iterator &it)
{
	con_t::iterator si;
	con_t::iterator ti;

	it = m_Map.end();

	unsigned int level = 0;

	for (si = m_Map.begin(); si != m_Map.end(); ++si)
	{
		assert((*si)->length >= 0);
		if ((*si)->offset + (off_t) (*si)->length <= offset) {
			continue;
		}

		if ((*si)->level > level)
		{
			it = si;
			level = (*si)->level;
		}

		if ((*si)->offset > offset + m_MaxLength) {
			return;
		}

		for (ti = si; ti != m_Map.end(); ++ti)
		{
			if ((*ti)->offset > offset) {
				return;
			}

			assert((*ti)->length >= 0);
			if (((*ti)->offset + (off_t) (*ti)->length <= offset)) {
				continue;
			}

			if ((*ti)->level > level)
			{
				it = ti;
				level = (*ti)->level;
			}
		}
	}
}

/* Returns Block that overlaps specified offset or higher offset */
bool LayerMap::Get(off_t offset, Block &rBlock, off_t &rLength)
{
//	std::cout << "State before Get called (looking offset: 0x" << hex << offset << ")" << std::endl << *this << std::endl;

	con_t::iterator it;

	find(offset, it);

	if (it == m_Map.end()) {
//		std::cout << "Get found nothing" << std::endl;
		return false;
	}

	// Return found block to caller.

	rBlock = **it;

//	std::cout << "Get found block: " << **it << std::endl;

	// Compute the maximal length uses can read from the Block
	// the 'it' iterator points to.

	rLength = length(it, offset);

//	std::cout << "Get found block's length: 0x" << std::hex << rLength << std::endl;

	return true;
}

