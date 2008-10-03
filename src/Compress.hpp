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
	typedef PARENT_COMPRESS Parent;

private:
	void restore(FileHeader& fh, const char *name);
	void restore(FileHeader& fh, int fd);
	void restore(LayerMap &lm, int fd);
	int store(int fd);
	void store(const FileHeader& fh, int fd);
	void store(const LayerMap& lm, int fd, off_t offset, const CompressionType& type);

	off_t writeCompressed(LayerMap& lm, off_t offset, off_t coffset, const char *buf, size_t size, int fd);
	ssize_t readCompressed(char *buf, size_t size, off_t offset, int fd);
	off_t copy(int readFd, off_t writeOffset, int writeFd, LayerMap& writeLm);

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
	Compress(const struct stat *st, const char *name);
	~Compress();

	int open(const char *name, int flags);

	int release(const char *name);

	int unlink(const char *name);

	int truncate(const char *name, off_t size);

	int getattr(const char *name, struct stat *st);

	ssize_t read(char *buf, size_t size, off_t offset);

	ssize_t write(const char *buf, size_t size, off_t offset);

	bool isCompressed() { return m_IsCompressed; }
	void setCompressed(bool compressed) { m_IsCompressed = compressed; if (!compressed) m_RawFileSize = 0; }
	bool isCompressedOnlyWith(CompressionType& type);
};

#endif

