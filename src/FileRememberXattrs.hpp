/*
    (C) Copyright Milan Svoboda 2009.
    
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

#ifndef FILEREMEMBERXATTRS_HPP
#define FILEREMEMBERXATTRS_HPP

#include <map>
#include <string>

/** Class that copies all extended file attributes from one file
 *  to an another one.
 */
class FileRememberXattrs
{
	std::map<std::string, std::string> m_xattrs;
public:
	void read(int fd_source);
	void write(int fd_dest);
};

#endif

