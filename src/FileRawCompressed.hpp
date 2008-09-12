#ifndef FILERAWCOMPRESSED_HPP
#define FILERAWCOMPRESSED_HPP

#include "FileRaw.hpp"
#include "FileHeader.hpp"
//#include "CompressMap.hpp"
#include "LayerMap.hpp"
#include "TransformTable.hpp"

class FileRawNormal;

class FileRawCompressed : public FileRaw
{
private:
	/*
	 * Store header and index to the file descriptor.
	 */
	int store(int fd);
	int store(const char *name);
	
	int m_fd;
	
	FileHeader m_fh;

	LayerMap m_lm;

	off_t m_length;		// Raw size of file.

	bool m_store;		// If true, index and header is
				// really written to the file...

	Transform *m_transform;
public:
	FileRawCompressed(const FileHeader &fh, off_t length);
	~FileRawCompressed();

	/*
	 * Returns true if FileRawCompressed object is able
	 * to be converted into FileRawNormal object.
	 * Returns false otherwise.
	 */
	bool isTransformableToFileRawNormal();

	/*
	 * Function creates a FileRawNormal instance with the
	 * same parameters as pThis had. pThis is deleted
	 * afterwards.
	 */
	static FileRawNormal *TransformToFileRawNormal(FileRaw *pFr);
	
	int open(int fd);
	int close();

	ssize_t read(char *buf, size_t size, off_t offset);
	ssize_t write(const char *buf, size_t size, off_t offset);

	int truncate(const char *name, off_t size);
};

#endif

