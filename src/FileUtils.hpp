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

#include <sys/stat.h>

class FileUtils
{
	/*
	 * Change the access rights of the file specified
	 * by name, open it and then return original
	 * access rights back.
	 */
	static int force(const char *name, const struct stat &buf);
public:
	/*
	 * Open a regular file specified by name. If the caller has
	 * insufficient rights, the function tries to
	 * change the access rights of the file, open it and
	 * then return the original rights back.
	 */
	static int open(const char *name);

	static bool copy(int source, int dest);

	static bool isZeroOnly(const char *buf, size_t size);
};

