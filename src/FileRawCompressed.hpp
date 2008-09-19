#ifndef FILERAWCOMPRESSED_HPP
#define FILERAWCOMPRESSED_HPP

#include "FileRaw.hpp"
#include "FileHeader.hpp"
#include "LayerMap.hpp"

class FileRawNormal;

class FileRawCompressed : public FileRaw
{
private:
	/*
	 * Store header and index to the file descriptor.
	 */
	int store(int fd);
	void store(FileHeader& fh, const LayerMap& lm, int fd);
	void restore(LayerMap& lm, int fd);
	
	int m_fd;
	
	FileHeader m_fh;

	LayerMap m_lm;

	off_t m_length;		// Raw size of file.
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

	void DefragmentFast();
};

#endif

