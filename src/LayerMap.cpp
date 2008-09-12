#include <algorithm>
#include <cassert>
#include <rlog/rlog.h>
#include <errno.h>

#include "LayerMap.hpp"

ostream &operator<< (ostream &os, const LayerMap &rLm)
{
	rLm.Print(os);
	return os;
}

void LayerMap::Put(Block *pBl, bool bKeepLevel)
{
//	cout << "Put Block: " << *pBl << " into following LayerMap:" << endl;
//	cout << *this << endl;

	if (!bKeepLevel)
		pBl->level = m_MaxLevel++;

	con_t::iterator it = m_Map.insert(pBl);

//	cout << "State after Put: " << endl << *this << endl;
}

void LayerMap::Truncate(off_t length)
{
	cout << "Truncate following LayerMap to 0x" << hex << length << endl;
	cout << *this << endl;

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

	cout << "State after Truncate: " << endl << *this << endl;
}

/* Find the biggest leveled Block with the higher offset than current 'it' points to. */
void LayerMap::next(con_t::iterator &it)
{
//	cout << "State before next called" << endl << *this << endl;
	cout << "next, current block: " << **it << endl;

	if (it == m_Map.end())
		return;

	unsigned int offset = (*it)->offset;
	unsigned int length = (*it)->length;
	unsigned int level = (*it)->level;

	// Find the next Block to read from. It has to have these properties:
	// 	Higher offset then curent 'it' Block.
	// 	Higher level if it overlaps the current 'it' Block.
	// 	Offset higher than offset + length of the current Block.
	// Algorithm depends on sorting order as defined by ltCom struct of
	// this class.

	while (++it != m_Map.end())
	{
		cout << "next, scanning block: " << **it << endl;

		if ((*it)->offset != offset)
		{
			if ((*it)->level > level)
				break;
			if ((*it)->offset >= offset + length)
				break;
		}
	}

	if (it == m_Map.end())
		return;

	cout << "next, found Block with the highest level: " << **it << endl;
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

	return min(len1, len2);
}

// Find Block with these parameters:
//  - Overlaps the specified offset
//  - Has higher level from all Blocks that overlap this offset
//  - Has higher offset than specified offset

void LayerMap::find(off_t offset, con_t::iterator &it)
{
	it = m_Map.end();

	unsigned int level = 0;

	for (con_t::iterator si = m_Map.begin(); si != m_Map.end(); ++si)
	{
		if ((*si)->offset > offset)
		{
			// Bigger offset, check if we found nothing to return
			// the Block with higher offset.

			if (it == m_Map.end())
				it = si;
			return;
		}

		if ((*si)->offset + (*si)->length <= offset)
			continue;

		if ((*si)->level > level)
		{
			level = (*si)->level;
			it = si;
		}
	}
}

/* Returns Block that overlaps specified offset or higher offset */
bool LayerMap::Get(off_t offset, Block &rBlock, off_t &rLength)
{
	cout << "Get offset: 0x" << hex << offset << endl;
	cout << "State before Get called" << endl << *this << endl;

	con_t::iterator it;

	// Find Block with these parameters:
	//  - Overlaps the specified offset
	//  - Has higher level from all Blocks that overlap this offset
	//  - Has higher offset than specified offset

	find(offset, it);

	if (it == m_Map.end())
	{
		cout << "Get found nothing" << endl;
		return false;
	}

	// Return found block to caller.

	rBlock = **it;

	cout << "Get found block: " << **it << endl;

	// Determine the numer of bytes that user can read from the Block
	// the 'it' iterator points to.

	rLength = length(it, offset);

	cout << "Get found block's length: 0x" << hex << rLength << endl;

	return true;
}

//

int LayerMap::store(char *buf, size_t len) const
{
	int				r;
	con_t::const_iterator		it;

	assert (len >= m_Map.size() * Block::size());

	for (it = m_Map.begin(); it != m_Map.end(); ++it)
	{
		// The order of the pointers to the 'Block' in vector must
		// be preserved...
		// 
		r = (*it)->store(buf, len);
		if (r == -1)
			return -1;

		buf += r;
		len -= r;
	}
	return 0;
}

off_t LayerMap::store(int fd, off_t to) const
{
	size_t			 len;
	char			*buf;
	Block			*bl;

 	cout << __PRETTY_FUNCTION__ << endl << *this << endl;

	if (m_Map.empty())
	{
		rDebug("Empty map");
		return 0;
	}

	// Create buffer to store the index.
	//
	len = Block::size() * m_Map.size();
	buf = new (std::nothrow) char[len];
	if (!buf)
	{
		rError("No memory to allocate block of %d bytes",
				len);

		errno = -ENOMEM;
		return -1;
	}

	if (store(buf, len) == -1)
	{
		delete[] buf;

		return -1;
	}

	// Compress buffer with index and store it
	// to the disk. Reserve space for the Block describing
	// this compressed index.
	//
	bl = m_transform->store(fd, to + Block::size(),
				    to + Block::size(), buf, len);
	delete[] buf;

	if (!bl)
		return -1;

	// Store block describing compressed index.
	// 
	to = bl->store(fd, to);
	if (to == (off_t) -1)
		goto out;

	// Correct file position. We must add buffer's compressed
	// length...
	// 
	to += bl->clength;
out:
	delete bl;

	return to;
}

int LayerMap::restore(size_t count, const char *buf, size_t len)
{
	int	 r;
	Block	*bl;
	
	// Use 'count--', not '--count' !!
	// 
	while (count--)
	{
		bl = new (std::nothrow) Block();
		if (!bl)
		{
			rError("No memory to allocate object of Block class");

			errno = -ENOMEM;
			return -1;
		}

		r = bl->restore(buf, len);
		if (r == (off_t) -1)
		{
			delete bl;
			
			return -1;
		}
		
		buf += r;
		len -= r;
		
		Put(bl, true);
	}
	return 0;
}

off_t LayerMap::restore(int fd, off_t from)
{
	char			*buf;
	size_t			 count;
	Block			 bl;

	// Delete the old map before a new one is
	// restored from the disk.
	//
	Truncate(0);

	// Read block describing compressed index.
	// 
	from = bl.restore(fd, from);
	if (from == (off_t) -1)
		return -1;

	// Compute number of blocks in the index.
	//
	count = bl.length / Block::size();

	// There is no index (it's zero length file...)
	// 
	if (count == 0)
		return from;

	// Create buffer for uncompressed index.
	//
	buf = new (std::nothrow) char[bl.length];
	if (!buf)
	{
		rError("No memory to allocate block of %d bytes",
				bl.length);

		errno = -ENOMEM;
		return -1;
	}

	// Decompress index from file to buffer.
	//
	from = m_transform->restore(fd, &bl, from, buf, bl.length);
	if (from == (off_t) -1)
		goto out;

	// Retrieve blocks and store them in this
	// class.
	// 
	if (restore(count, buf, bl.length) == -1)
		from = -1;
out:	
	delete[] buf;
	
	return from;
}


