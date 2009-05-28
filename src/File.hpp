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

	ino_t		m_inode;

	// Reference counter. Used to open m_fd just once even
	// for multiple users. If value drops to zero, m_fd is
	// closed.
	//
	int m_refs;

private:
	// USED ONLY IN THIS CLASS PRIVATELY

	Mutex m_mutex;

	File(const File&);	// Private copy constructor.
public:
	std::string	m_name;

	File(const struct stat *st, const char *name);
	virtual ~File();

	void Lock(void) { m_mutex.Lock(); }
	void Unlock(void) { m_mutex.Unlock(); }

	ino_t getInode(void) const { return m_inode; }
	void  setInode(ino_t inode) { m_inode = inode; }
	std::string getName(void) const { return m_name; }

	int getattr(const char *name, struct stat *st);

	int unlink(const char *name);

	int truncate(const char *name, off_t size);

	int utimens(const char *name, const struct timespec tv[2]);
	
	int open(const char *name, int flags);

	/**
	 * Read `size` of bytes from the file with offset `offset`
	 * and store them to the `buf`
	 */
	ssize_t read(char *buf, size_t size, off_t offset) const;
	
	/**
	 * Write `size` of bytes from the buffer `buf` and store them
	 * to the file with offset `offset`
	 */
	ssize_t write(const char *buf, size_t size, off_t offset);
	
	int flush(const char *name);
	
	int release(const char *name);

	int fdatasync(const char *name);

	int fsync(const char *name);
};

#endif

