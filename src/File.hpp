#ifndef FILE_HPP
#define FILE_HPP

#include "Mutex.hpp"

#include <sys/types.h>

#include <string>

using namespace std;

class File
{
protected:
	// SHARED WITH CLASSES THAT INHERITS FROM THIS CLASS

	// File descriptor used to read/write/etc...
	//
	int m_fd;

	ino_t m_inode;

	// Reference counter. Used to open m_fd just once even
	// for multiple users. Is value drops to zero, m_fd is
	// closed.
	//
	int m_refs;

private:
	// USED ONLY IN THIS CLASS PRIVATELY

	Mutex m_mutex;

public:
	File(const struct stat *st);
	virtual ~File();

	void Lock(void) { m_mutex.Lock(); }
	void Unlock(void) { m_mutex.Unlock(); }

	ino_t getInode(void) const { return m_inode; }

	string m_FileName;

	virtual int getattr(const char *name, struct stat *st);

	virtual int unlink(const char *name);

	virtual int truncate(const char *name, off_t size);

	virtual int utime(const char *name, struct utimbuf *buf);
	
	virtual int open(const char *name, int flags);

	/**
	 * Read `size` of bytes from the file with offset `offset`
	 * and store them to the `buf`
	 */
	virtual ssize_t read(char *buf, size_t size, off_t offset);
	
	/**
	 * Write `size` of bytes from the buffer `buf` and store them
	 * to the file with offset `offset`
	 */
	virtual ssize_t write(const char *buf, size_t size, off_t offset);
	
	virtual int flush(const char *name);
	
	virtual int release(const char *name);

	virtual int fdatasync(const char *name);

	virtual int fsync(const char *name);
};

#endif

