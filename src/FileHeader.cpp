#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <cassert>

#include "config.h"

// Milan's TODO: This may be not on all supported platforms.
//
#include <sys/types.h>
#if defined(HAVE_ATTR_XATTR_H)
#  include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
#  include <sys/xattr.h>
#endif
#include <rlog/rlog.h>

#include "ByteOrder.hpp"
#include "FileHeader.hpp"
#include "FileUtils.hpp"

const char *FileHeader::m_pExtAttrName = "user.fusecompress";

// Store in little-endian byteorder on disk.
// 
struct packed_header
{
	uint8_t		id[3];
	uint8_t		type;
	uint64_t	size;
	uint64_t	index;

} __attribute__((packed));

FileHeader::FileHeader()
{
	// FuseCompress identification
	// 
	id[0] = '\037';
	id[1] = '\135';
	id[2] = '\211';

	// Uncompressed type
	// 
	type = 0;

	// Zero size
	// 
	size = 0;

	// Index is not present
	//
	index = 0;
}

bool FileHeader::restore(const char *name)
{
	int                  r;
	int                  fd;
	struct packed_header ph;

	r = getxattr(name, m_pExtAttrName, &ph, sizeof ph);
	if (r != sizeof ph)
	{
		// Header is not stored in extended attribute,
		// read header from the file.
		//
		if ((fd = FileUtils::open(name)) >= 0)
		{
			restore(fd, 0);

			// If the file is compressed by fusecompress and
			// user wants to use extended attributes, create
			// extended attribute with the header.
			// 
			if (isValid())
			{
				r = fsetxattr(fd, m_pExtAttrName, &ph, sizeof ph, 0);
				if (r == -1)
				{
					if (errno != ENOTSUP)
					{
						rWarning("Cannot write extended attribute (%s)", strerror(errno));
					}
				}
			}

			::close(fd);

			return isValid();
		}

		return false;
	}

	id[0] = ph.id[0];
	id[1] = ph.id[1];
	id[2] = ph.id[2];

	if (isValid())
	{
		// Update only when header is valid.
		// 
		type = ph.type;
		size = le64_to_cpu(ph.size);
		index = le64_to_cpu(ph.index);

		return true;
	}
	return false;
}

off_t FileHeader::restore(int fd, off_t from)
{
	int			r;
	struct packed_header	ph;
	
	r = pread(fd, &ph, sizeof ph, from);
	if (r != sizeof ph)
	{
		id[0] = 0;
		id[1] = 0;
		id[2] = 0;

		return -1;
	}
	
	id[0] = ph.id[0];
	id[1] = ph.id[1];
	id[2] = ph.id[2];

	if (isValid())
	{
		// Update only when header is valid.
		// 
		type = ph.type;
		size = le64_to_cpu(ph.size);
		index = le64_to_cpu(ph.index);
	}

	return from + r;
}

off_t FileHeader::store(int fd, off_t to)
{
	int			r;
	struct packed_header	ph;

	// Store only valid header.
	// 
	assert (isValid());

	ph.id[0] = id[0];
	ph.id[1] = id[1];
	ph.id[2] = id[2];
	
	ph.type = type;
	ph.size = cpu_to_le64(size);
	ph.index = cpu_to_le64(index);

	// Save the header to the extended attribute
	// to speed up getattr...
	//
	r = fsetxattr(fd, m_pExtAttrName, &ph, sizeof ph, 0);
	if (r == -1)
	{
		if (errno != ENOTSUP)
		{
			rWarning("Cannot write extended attribute (%s)", strerror(errno));
		}
	}

	r = pwrite(fd, &ph, sizeof ph, to);
	if (r != sizeof ph)
		return -1;

	return to + r;
}

