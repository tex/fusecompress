/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FUSECOMPRESS_H
#define FUSECOMPRESS_H

#include <fuse.h>
#include <string>

class FuseCompress
{
	inline static const char *getpath(const char *path);
	static void replace(std::string& path, std::string part, std::string newpart);
	
	static int readlink (const char *, char *, size_t);
	static int getattr (const char *, struct stat *);
	static int getdir (const char *, fuse_dirh_t, fuse_dirfil_t);
	static int mknod (const char *, mode_t, dev_t);
	static int mkdir (const char *, mode_t);
	static int unlink (const char *);
	static int rmdir (const char *);
	static int symlink (const char *, const char *);
	static int rename (const char *, const char *);
	static int link (const char *, const char *);
	static int chmod (const char *, mode_t);
	static int chown (const char *, uid_t, gid_t);
	static int truncate (const char *, off_t);
	static int utimens (const char *, const struct timespec tv[2]);
	static int open (const char *, struct fuse_file_info *);
	static int read (const char *, char *, size_t, off_t, struct fuse_file_info *);
	static int write (const char *, const char *, size_t, off_t,struct fuse_file_info *);
	static int flush (const char *, struct fuse_file_info *);
	static int release (const char *, struct fuse_file_info *);
	static int fsync (const char *, int, struct fuse_file_info *);
	static int setxattr (const char *, const char *, const char *, size_t, int);
	static int getxattr (const char *, const char *, char *, size_t);
	static int listxattr (const char *, char *, size_t);
	static int removexattr (const char *, const char *);
	static int opendir (const char *, struct fuse_file_info *);
	static int readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
	static int releasedir (const char *, struct fuse_file_info *);
	static int fsyncdir (const char *, int, struct fuse_file_info *);
	static void *init (struct fuse_conn_info *);
	static void  destroy (void *);
	static int access (const char *, int);
	static int create (const char *, mode_t, struct fuse_file_info *);
	static int ftruncate (const char *, off_t, struct fuse_file_info *);
	static int fgetattr (const char *, struct stat *, struct fuse_file_info *);
	static int statfs (const char *, struct statvfs *);

	struct fuse_operations m_ops;

public:
	FuseCompress();
	~FuseCompress();
	
	int Run(DIR *dir, int argc, const char **argv);
};

#endif

