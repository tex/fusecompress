#ifndef FILEREMEMBERXATTRS_HPP
#define FILEREMEMBERXATTRS_HPP

#include <map>
#include <string>

/** Class that copies all extended file attributes from one file
 *  to a another one.
 */
class FileRememberXattrs
{
	std::map<std::string, std::string> m_xattrs;
public:
	void read(int fd_source);
	void write(int fd_dest);
};

#endif

