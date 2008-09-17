#ifndef PLAINMAP_HPP
#define PLAINMAP_HPP

#include <sys/types.h>

#include <map>
#include <utility>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

class LinearMap
{
	struct Buffer
	{
		Buffer(const char *buf, size_t size) {
			this->buf = new char[size];
			this->size = size;
			memcpy(this->buf, buf, size);
		};

		~Buffer() {
			delete[] this->buf;
		};

		char	*buf;
		size_t	 size;
	};

	typedef map<off_t, Buffer *>	con_t;
	
	con_t	m_map;

	size_t find_total_length(con_t::const_iterator it) const;

	void copy_all(con_t::iterator it, char *buf, ssize_t len);

	con_t::const_iterator get(off_t offset) const;

	void inline Check() const;
public:
	void Print(ostream &os) const;

	LinearMap();
	~LinearMap();

	/**
	 */
	int put(const char *buf, size_t size, off_t offset);

	/**
	 * Input:
	 * @param offset
	 * @param size
	 * 
	 * Output:
	 * @param buf
	 * @param size
	 */
	off_t get(off_t offset, char **buf, size_t *size) const;

	/**
	 * Block is returned only if it fits some criterias. Like
	 * that the block is continuous and has at
	 * least some length (eg. 1MiB).
	 *
	 * @param force - if true it returns block even if it doesn't fit
	 *                any criterias.
	 * 
	 * @return true if some block is returned
	 */
	bool erase(off_t *offset, char **buf, size_t *size, bool force);

	bool empty() { return m_map.empty(); };
	
	void truncate(off_t size);
};

ostream &operator<<(ostream &os, const LinearMap &rLinearMap);

#endif

