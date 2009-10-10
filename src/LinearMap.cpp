/*
    (C) Copyright Milan Svoboda 2009.
    
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

#include "LinearMap.hpp"

#include <errno.h>
#include <stdlib.h>

#include <cassert>
#include <rlog/rlog.h>

#include <boost/io/ios_state.hpp>

extern size_t g_BufferedMemorySize;

LinearMap::LinearMap()
{
}

LinearMap::~LinearMap()
{
}

LinearMap::Buffer *LinearMap::merge(Buffer *prev, Buffer *next) const
{
	Buffer *buffer = new (std::nothrow) Buffer(prev->buf, prev->size,
	                                           next->buf, next->size);
	assert(buffer);

	delete prev;
	delete next;

	return buffer;
}

void LinearMap::insert(off_t offset, const char *buf, size_t size)
{
	Buffer *buffer = new Buffer(buf, size);
	assert(buffer);

	con_t::iterator it = m_map.lower_bound(offset);

	if (it != m_map.begin())
	{
		con_t::iterator prev = it; --prev;

		if (prev->first + prev->second->size == offset)
		{
			// Merge with previous Buffer
			//
			offset = prev->first;
			size += prev->second->size;
			Buffer *pb = prev->second;
			m_map.erase(prev);
			buffer = merge(pb, buffer);
		}
	}
	if (it != m_map.end())
	{
		if (it->first == offset + size)
		{
			// Merge with next Buffer
			//
			size += it->second->size;
			Buffer *nb = it->second;
			m_map.erase(it);
			buffer = merge(buffer, nb);
		}
	}
	m_map[offset] = buffer;
}

int LinearMap::put(const char *buf, size_t size, off_t offset)
{
	char	*tmp;
	off_t	 tmp_offset;
	size_t	 tmp_size;
	size_t	 len;
	size_t	 off;

	assert (size > 0);

	while (size > 0)
	{
		// Check if new buffer overlaps some existing
		// Buffers already there...
		//
		tmp_offset = get(offset, &tmp, &tmp_size);
		
		if (tmp_offset == -1)
		{
			// There is no Buffer there. Cheat and
			// set tmp_offset so next 'if' creates
			// Buffer with the 'size'.
			// 
			tmp_offset = offset + size;
		}
		
		if (tmp_offset > offset)
		{
			// Create Buffer for offsets between
			// offset to tmp_offset and remember it.
			//
			len = std::min(tmp_offset - offset, (off_t) size);
			insert(offset, buf, len);
		}
		else
		{
			// There is already Buffer on this offset or
			// before (with the lenght that overlaps offset
			// we are looking for).
			// 
			// tmp_offset
			// |---tmp_size---|(----------|)
			//              offset
			//              |---size---|

			// This is difference between offset we search and
			// offset we got...
			// 
			off = (size_t) (offset - tmp_offset);
			len = std::min(size, tmp_size - off);
			memcpy(tmp + off, buf, len);
		}
		offset += len;
		buf    += len;
		size   -= len;
	}

	Check();
	return 0;
}

/**
 * Return iterator that points to the Buffer that
 * covers the offset or higher offsets.
 */ 
LinearMap::con_t::const_iterator LinearMap::get(off_t offset) const
{
	if (m_map.empty())
		return m_map.end();

	con_t::const_iterator it = m_map.lower_bound(offset);

	if (it != m_map.begin())
	{
		// There is a block before.
		//
		--it;

		assert(it->second->size >= 0);
		if (it->first + (off_t) it->second->size > offset)
		{
			return it;
		}
		++it;
	}

	return it;
}

/**
 * Find Buffer that covers the offset or it's start is higher than
 * offset. Returns it's parameters.
 */ 
off_t LinearMap::get(off_t offset, char **buf, size_t *size) const
{
	con_t::const_iterator	 it;
	Buffer			*buffer;

	it = get(offset);

	if (it == m_map.end())
		return -1;

	buffer = it->second;
	
	*buf = buffer->buf;
	*size = buffer->size;

	return it->first;
}

void LinearMap::truncate(off_t size)
{
	if (m_map.empty())
		return;

	con_t::iterator it = m_map.lower_bound(size);

	// Check if previous block overlaps the required size.
	//
	if (it != m_map.begin())
	{
		// There is a block before.
		//
		--it;

		assert(it->second->size >= 0);
		if (it->first + (off_t) it->second->size > size)
		{
			// Truncate this Buffer.
			//
			off_t off = (it->first + it->second->size) - size;
			
			Buffer *buffer = new (std::nothrow)
			                 Buffer(it->second->buf,
			                        it->second->size - off);

			delete it->second;

			// Replace old Buffer with new truncated one.
			//
			it->second = buffer;
		}

		++it;
	}

	// Delete all Buffers after this iterator
	// 
	while (it != m_map.end())
	{
		delete it->second;
		m_map.erase(it++);
	}

	Check();
}

bool LinearMap::erase(off_t *offset, char **buf, size_t *size, bool force)
{
	size_t totalsize = 0;
	con_t::iterator it = m_map.begin();

	if (force == false)
	{
		// Select the best block(s) to write down.

		while (it != m_map.end())
		{
			if (it->second->size > g_BufferedMemorySize)
				break;
			totalsize += it->second->size;
			++it;
		}

		if (it == m_map.end())
		{
			if (totalsize > 2 * g_BufferedMemorySize)
			{
				it = m_map.begin();
			}
		}
	}

	if (it != m_map.end())
	{
		*offset = it->first;
		it->second->release(buf, size);
		delete it->second;
		m_map.erase(it);
		return true;
	}
	return false;
}

void LinearMap::Check() const
{
#ifdef NDEBUG
	return;
#else
	con_t::const_iterator it;

	for (it = m_map.begin(); it != m_map.end(); ++it)
	{
		con_t::const_iterator ni = it;

		if (++ni != m_map.end())
		{
			if (it->first + it->second->size > ni->first)
			{
				std::cout << *this << std::endl;
				abort();
			}
		}
	}
#endif
}

std::ostream &operator<<(std::ostream &os, const LinearMap &rLm)
{
	boost::io::ios_flags_saver ifs(os);
	os << std::hex;
 
	LinearMap::con_t::const_iterator it;
	for (it = rLm.m_map.begin(); it != rLm.m_map.end(); ++it)
	{
		os << "offset: 0x" << it->first << ", size: 0x" << it->second->size << std::endl;
	}
	return os;
}

