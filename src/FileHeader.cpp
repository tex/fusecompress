#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <cassert>

#include "config.h"

#include <sys/types.h>
#include <rlog/rlog.h>

#include "FileHeader.hpp"
#include "FileUtils.hpp"

FileHeader::FileHeader(bool valid)
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

