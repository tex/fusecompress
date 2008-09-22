#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <sys/types.h>
#include <pthread.h>

#include <set>
#include <map>

#include "Memory.hpp"
#include "Compress.hpp"

#include "Mutex.hpp"

//typedef File PARENT_CFILE;
typedef Memory PARENT_CFILE;
//typedef Compress PARENT_CFILE;

/**
 * Helper class that's only goal is to remember how many users currently
 * use it. m_cref counter is protected with mutex in FileManager class.
 */
class CFile : public PARENT_CFILE
{
private:
	friend class FileManager;

	/**
	 * Number of users that currently use this File.
	 */
	int m_crefs;

	mode_t m_mode;

	CFile();				// No default constructor
	CFile(const CFile &);			// No copy constructor
	CFile& operator=(const CFile &);	// No assign operator


public:
	CFile(const struct stat *st) :
		PARENT_CFILE (st),
		m_crefs (1)
	{ };
};

class FileManager
{
	/**
	 * Using struct instead of class only because its
	 * default access is public.
	 */
	struct ltFile
	{
		bool operator()(const File *file1, const File *file2) const
		{
			return (file1->getInode() < file2->getInode());
		}
	};
	
	set<File *, ltFile> m_files;

	/**
	 * Protects m_files and every m_refs in CFile type instancies
	 */
	Mutex m_mutex;

public:
	FileManager();
	~FileManager();

	void Lock() { m_mutex.Lock(); }
	void Unlock() { m_mutex.Unlock(); }
	
	/**
	 * Returns pointer to CFile instance that does all operations on
	 * the requested name. It works even with hardlinks because name
	 * is translated to inode number and that number is used as a key.
	 *
	 * @param name
	 * @param create - if true, a new instance is created (with reference
	 *                          counter set to one) if there is no
	 *                          one in the database (set m_files). If there
	 *                          is one, it's reference counter is increased.
	 *                          
	 *                 if false, returns a null if there is no one in
	 *                           the database. If there is one, it's reference
	 *                           counter is not increased.
	 */
	CFile *Get(const char *name, bool create = true);

	void  Put(CFile *file);
};

#endif

