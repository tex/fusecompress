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

#ifndef BLOCKHPP
#define BLOCKHPP

#include <ostream>

#include <boost/serialization/version.hpp>

#include "CompressionType.hpp"

class Block
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & offset & coffset;
		ar & length & olength;
		ar & clength;
		ar & level;
		ar & type;
	}
public:
	Block(const Block &src) :
		offset (src.offset), coffset (src.coffset), length (src.length),
		olength (src.olength), clength(src.clength), level (src.level), type (src.type)
		{ }

	Block(off_t offset, size_t length, off_t coffset, size_t olength, size_t clength, unsigned int level, unsigned char type) :
		offset (offset), coffset (coffset), length (length),
		olength (olength), clength(clength), level (level), type (type)
		{ }

	Block()
	:	offset (0), coffset (0), length (0),
		olength (0), clength (0), level (0), type (CompressionType::NONE)
		{ }

	Block(off_t offset, size_t length)
	:	offset (offset), coffset (0), length (length),
		olength (length), clength (0), level (0), type(CompressionType::NONE)
		{ }

	Block(off_t offset, size_t length, unsigned int level)
	:	offset (offset), coffset (0), length (length),
		olength (length), clength (0), level (level), type(CompressionType::NONE)
		{ }

	Block(CompressionType type)
	:	offset (0), coffset (0), length (0),
		olength (0), clength (0), level (0), type (type)
		{ }

	~Block() {};

	friend std::ostream &operator<< (std::ostream& os, const Block& rBl);

	off_t offset, coffset;
	size_t length, olength;
	size_t clength;

	unsigned int level;

	CompressionType type;
};

BOOST_CLASS_VERSION(Block, 0)

#endif

