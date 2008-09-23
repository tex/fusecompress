#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <cassert>

#include "config.h"

#include <sys/types.h>
#if defined(HAVE_ATTR_XATTR_H)
#  include <attr/xattr.h>
#elif defined(HAVE_SYS_XATTR_H)
#  include <sys/xattr.h>
#endif
#include <rlog/rlog.h>

#include "FileHeader.hpp"
#include "FileUtils.hpp"
/*
const char *FileHeader::m_pExtAttrName = "user.fusecompress";
*/

FileHeader::FileHeader(bool valid) :
	type (CompressionType::ZLIB)
{
	if (valid)
	{
		// FuseCompress identification
		// 
		id_0 = '\037';
		id_1 = '\135';
		id_2 = '\211';
	}
	else
	{
		id_0 = 0;
		id_1 = 0;
		id_2 = 0;
	}

	// Zero size
	// 
	size = 0;

	// Index is not present
	//
	index = 0;
}

