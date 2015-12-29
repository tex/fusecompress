/*
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    FuseCompress is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FuseCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LinearMap_HPP
#define LinearMap_HPP

#include <sys/types.h>

#include <map>
#include <utility>
#include <iostream>
#include <cstring>

class LinearMap
{
	struct Buffer
	{
		Buffer(const char *buf, size_t size) {
			this->size = size;
			this->buf = new char[this->size];
			memcpy(this->buf, buf, this->size);
		};
		Buffer(const char *buf1, size_t size1,
		       const char *buf2, size_t size2) {
			this->size = size1 + size2;
			this->buf = new char[this->size];
			memcpy(this->buf, buf1, size1);
			memcpy(this->buf + size1, buf2, size2);
		};

		~Buffer() {
			delete[] this->buf;
		};

		void release(char **buf, size_t *size) {
			*buf = this->buf;
			*size = this->size;
			this->buf = NULL;
			this->size = 0;
		}

		char	*buf;
		size_t	 size;
	};

	typedef std::map<off_t, Buffer *>	con_t;
	
	con_t	m_map;

	con_t::const_iterator get(off_t offset) const;

	void inline Check() const;

	Buffer *merge(Buffer *prev, Buffer *next) const;
	void insert(off_t offset, const char *buf, size_t size);

public:
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

	friend std::ostream &operator<<(std::ostream &os, const LinearMap &rLinearMap);
};

#endif

