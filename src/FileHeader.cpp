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

