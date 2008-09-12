#ifndef FILEHEADER_HPP
#define FILEHEADER_HPP

#include <sys/types.h>

using namespace std;

class FileHeader
{
public:
	static const int HeaderSize = (sizeof(char)) * 3 +
	                              (sizeof(unsigned char)) +
	                              (sizeof(off_t) * 2);

	FileHeader();
	~FileHeader() {};

	/*
	 * Object persistion, store object to file.
	 */
	off_t store(int fd, off_t to);
	
	/*
	 * Object persistion, restore object from the file.
	 */
	off_t restore(int fd, off_t from);

	/*
	 * Object persistion, restore object from the file.
	 */
	bool restore(const char *name);

	/*
	 * Identify header.
	 *
	 * Return:
	 * 	- True if header is correct (file is FuseCompress format).
	 * 	- False if header is not correct.
	 */
	inline bool isValid() const
	{
		if ((id[0] != '\037') || (id[1] != '\135') || (id[2] != '\211'))
			return false;
		return true;
	}

	char		id[3];	// FuseCompress identification
	unsigned char	type;	// Compression type
	off_t		size;	// Length of the uncompressed file
	off_t		index;	// Position of the index in the compressed file
				// (0 - index is not present in the file)
private:
	static const char *m_pExtAttrName;
};

#endif

