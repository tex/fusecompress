#include "LayerMap.hpp"

#include <algorithm>
#include <cassert>

#include <boost/io/ios_state.hpp>
#include <rlog/rlog.h>

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

	// Preserve a level if already set.
	//
	if (!bKeepLevel)
		pBl->level = m_MaxLevel++;
	con_t::iterator it = m_Map.insert(pBl);
	if (m_MaxLength < pBl->length)
		m_MaxLength = pBl->length;

//	cout << "State after Put: " << endl << *this << endl;
}

void LayerMap::Truncate(off_t length)
{
//	std::cout << "Truncate following LayerMap to 0x" << std::hex << length << std::endl;
//	std::cout << *this << std::endl;

	con_t::iterator it = m_Map.begin();

	while (it != m_Map.end())
	{
		if ((*it)->offset >= length)
		{
			delete (*it);
			m_Map.erase(it++);
			continue;
		}
		assert((*it)->length > 0);
		if ((*it)->offset + (off_t) (*it)->length > length)
		{
			(*it)->length = length - (*it)->offset;
		}
		++it;
	}
//	std::cout << "State after Truncate: " << std::endl << *this << std::endl;
}

/* Find the biggest leveled Block with the higher offset than current 'it' points to. */
void LayerMap::next(con_t::iterator &it)
{
//	cout << "State before next called" << endl << *this << endl;
//	cout << "Current block: " << **it << endl;

	if (it == m_Map.end())
		return;

	unsigned int offset = (*it)->offset;
	unsigned int level = (*it)->level;

	// Find the first Block on the higher offset with the bigger level.
	// Algorithm depends on sorting order as defined by ltCom struct of
	// this class.

	while (++it != m_Map.end())
	{
		if (((*it)->offset != offset) && ((*it)->level > level))
			break;
	}

	if (it == m_Map.end())
		return;

//	cout << "Found Block with the highest level: " << **it << endl;
}

/* Returns number of bytes that can be read from the offset from the Block specified by 'it' iterator. */
unsigned int LayerMap::length(con_t::iterator &it, off_t offset)
{
	if ((*it)->offset > offset)
		// Block starts after the offset, return 0 to
		// indicate that.
		return 0;

	unsigned int len1 = (*it)->length - (offset - (*it)->offset);

	con_t::iterator ni = it;

	while ((ni != m_Map.end()) && ((*ni)->offset <= offset))
		next(ni);

	if (ni == m_Map.end())
		return len1;
	if ((*ni)->level < (*it)->level)
		return len1;

	unsigned int len2 = (*ni)->offset - offset;

	return std::min(len1, len2);
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
		if ((*si)->offset + (off_t) (*si)->length <= offset)
			continue;

		if ((*si)->level > level)
			it = si;

		if ((*si)->offset > offset + m_MaxLength)
			return;

		for (ti = si; ti != m_Map.end(); ++ti)
		{
			if ((*ti)->offset > offset)
				return;

			assert((*ti)->length >= 0);
			if (((*ti)->offset + (off_t) (*ti)->length <= offset))
				continue;

			if ((*ti)->level > level)
			{
				it = ti;
				level = (*ti)->level;
			}
		}
		level = (*si)->level;
	}
}

/* Returns Block that overlaps specified offset or higher offset */
bool LayerMap::Get(off_t offset, Block &rBlock, off_t &rLength)
{
//	std::cout << "State before Get called" << std::endl << *this << std::endl;

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

