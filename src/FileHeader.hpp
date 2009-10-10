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

#ifndef FILEHEADER_HPP
#define FILEHEADER_HPP

#include <sys/types.h>
#include <cassert>

#include <boost/serialization/access.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/version.hpp>

#include <iostream>

#include "CompressionType.hpp"

using namespace std;

class FileHeader
{
private:
	friend class boost::serialization::access;

	template<class Archive>
	void load(Archive &ar, const unsigned /*version*/)
	{
		ar >> id_0 >> id_1 >> id_2;

		if (isValid())
		{
			ar >> size >> index >> type;
		}
	}
	template<class Archive>
	void save(Archive &ar, const unsigned /*version*/) const
	{
		assert(isValid());

		ar << id_0 << id_1 << id_2;
		ar << size << index << type;
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
	FileHeader(bool valid = true);

	FileHeader(const FileHeader& src) :
		id_0(src.id_0),
		id_1(src.id_1),
		id_2(src.id_2),
		size(src.size),
		index(src.index),
		type(src.type)
	{};

	FileHeader& operator=(const FileHeader& src)
	{
		id_0 = src.id_0;
		id_1 = src.id_1;
		id_2 = src.id_2;
		size = src.size;
		index = src.index;
		type = src.type;

		return *this;
	}

	/*
	 * Identify header.
	 *
	 * Return:
	 * 	- True if header is correct (file is FuseCompress format).
	 * 	- False if header is not correct.
	 */
	inline bool isValid() const
	{
		if ((id_0 != '\037') || (id_1 != '\135') || (id_2 != '\211'))
			return false;
		return true;
	}

	signed char	id_0;	// FuseCompress identification
	signed char	id_1;	// (these meant to be unsigned, however I oversight
	signed char	id_2;	//  that boost::archive stores char type as signed)
	off_t		size;	// Length of the uncompressed file
	off_t		index;	// Position of the index in the compressed file
				// (0 - index is not present in the file)
	CompressionType	type;	// Compression type of the index

	// Minimal size:
	//
	// 1., ..., 6. byte: FuseCompress identification
	// 7. byte         : size (=0 => 1 byte)
	// 8. byte         : index (=0 => 1 byte)
	// 9., ... byte    : compression type

	static const int MinSize = 3 + 1 + 1 + CompressionType::MinSize;

	// Current maximal size:
	//
	// 1., ..., 6. byte: FuseCompress identification
	// 7., ... byte    : size (>0 => 1 byte + up to 8 bytes)
	// ..., ... byte   : index (>0 => 1 byte + up to 8 bytes)
	// ..., ... byte   : compression type

	static const int MaxSize = 3 + 1 + 8 + 1 + 8 + CompressionType::MaxSize;
};

// Don't need versioning info for the FileHeader.

BOOST_CLASS_IMPLEMENTATION(FileHeader, boost::serialization::object_serializable)

// Don't track instances of this class. Users creates
// them on the stack.

BOOST_CLASS_TRACKING(FileHeader, boost::serialization::track_never) 

#endif

