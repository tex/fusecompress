/*
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

#ifndef COMPRESS_HPP
#define COMPRESS_HPP

#include "File.hpp"
#include "FileHeader.hpp"
#include "LayerMap.hpp"
#include "CompressionType.hpp"

#include <sys/types.h>

typedef File PARENT_COMPRESS;

/**
 */
class Compress : public PARENT_COMPRESS
{
private:
	typedef PARENT_COMPRESS Parent;

	void restoreFileHeader(const char *name);
	void restoreLayerMap();

	/**
	 * Store (save) the layer map and the file header.
	 *
	 * @returns 0 on success, -1 otherwise and errno set to -EIO.
	 */
	int store();

	/**
	 * Store (save) the file header m_fh.
	 *
	 * @throws boost::iostreams exception on error.
	 */
	void storeFileHeader() const;

	/**
	 * Store (save) the layer map m_lm (to offset m_RawFileSize) using
	 * compression as requested by 'm_fh.type'.
	 *
	 * @throws boost::iostreams exception on error.
	 */
	void storeLayerMap();

	off_t writeCompressed(LayerMap& lm, off_t offset, off_t coffset, const char *buf, size_t size, int fd, off_t rawFileSize);
	off_t readBlock(int fd, const Block& block, off_t size, off_t len, off_t offset, char *buf) const;
	ssize_t readCompressed(char *buf, size_t size, off_t offset, int fd) const;
	off_t copy(int readFd, off_t writeOffset, int writeFd, LayerMap& writeLm);
	off_t cleverCopy(int readFd, off_t writeOffset, int writeFd, LayerMap& writeLm);

	void DefragmentFast();

	// Length of the lower file
	// (not as seen by the user via fuse mount point).
	//
	off_t	 m_RawFileSize;

	bool	 m_IsCompressed;

	// Items used when a file is compressed.
	//
	FileHeader m_fh;
	LayerMap   m_lm;

	Compress(const Compress &);		// No copy constructor
	Compress();				// No default constructor
	Compress& operator=(const Compress &);	// No assign operator
public:

	friend ostream &operator<<(ostream &os, const Compress &rCompress);

	Compress(const struct stat *st, const char *name);
	~Compress();

	int open(const char *name, int flags);

	int release(const char *name);

	int unlink(const char *name);

	int truncate(const char *name, off_t size);

	int getattr(const char *name, struct stat *st);

	ssize_t read(char *buf, size_t size, off_t offset) const;

	ssize_t write(const char *buf, size_t size, off_t offset);

	bool isCompressed() { return m_IsCompressed; }
	void setCompressed(bool compressed) { m_IsCompressed = compressed; if (!compressed) m_RawFileSize = 0; }
	bool isCompressedOnlyWith(CompressionType& type);
};

#endif

