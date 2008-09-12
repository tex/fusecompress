#include "LinearMap.hpp"

#include <errno.h>
#include <stdlib.h>

#include <cassert>
#include <rlog/rlog.h>

extern unsigned int g_BufferedMemorySize;

LinearMap::LinearMap()
{
}

LinearMap::~LinearMap()
{
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
			len = min(tmp_offset - offset, (off_t) size);
			Buffer *buffer = new (std::nothrow) Buffer(buf, len);
			if (!buffer)
			{
				rError("No memory to allocate block of %d bytes",
						len);

				errno = -ENOMEM;
				return -1;
			}
			m_map[offset] = buffer;
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
			len = min(size, tmp_size - off);
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
		--it;
		if (it->first + it->second->size > offset)
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

		if (it->first + it->second->size > size)
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

size_t LinearMap::find_total_length(con_t::const_iterator it) const
{
	size_t	size = 0;
	off_t	from = it->first;

	for (; it != m_map.end() && it->first == from; it++)
	{
		size += it->second->size;
		from += it->second->size;
	}
	
	return size;
}

void LinearMap::copy_all(con_t::iterator it, char *buf, ssize_t len)
{
	assert (it != m_map.end());

	off_t					 from = it->first;
	Buffer					*buffer;
	
	while (it != m_map.end() && it->first == from)
	{
		buffer = it->second;
		
		memcpy(buf, buffer->buf, buffer->size);
		buf  += buffer->size;
		from += buffer->size;
		len  -= buffer->size;

		delete buffer;
		m_map.erase(it++);
	}
	assert (len == 0);
}

bool LinearMap::erase(off_t *offset, char **buf, size_t *size, bool force)
{
	size_t		len;
	con_t::iterator	it;

	*offset = 0;
	
	while ((it = m_map.lower_bound(*offset)) != m_map.end())
	{
		*offset = it->first;

		len = find_total_length(it);
		assert (len > 0);
		
		if ((force == true) || (len >= g_BufferedMemorySize))
		{
			*size = len;
			*buf = new (std::nothrow) char[len];
			assert (*buf);
		
			copy_all(it, *buf, len);

			return true;
		}
		
		*offset += len;
	}
	Check();
	return false;
}

void LinearMap::Print(ostream &os) const
{
	con_t::const_iterator it;

	for (it = m_map.begin(); it != m_map.end(); ++it)
	{
		os << "offset: 0x" << hex << it->first << ", size: 0x" << hex << it->second->size << endl;
	}
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
				Print(cout);
				abort();
			}
		}
	}
#endif
}

ostream &operator<<(ostream &os, const LinearMap &rLinearMap)
{
	rLinearMap.Print(os);
	return os;
}

