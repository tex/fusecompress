#ifndef BLOCKHPP
#define BLOCKHPP

#include <ostream>

#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include "CompressionType.hpp"

//extern CompressionType g_CompressionType;

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

	void Print(std::ostream &os) const;

	off_t offset, coffset;
	size_t length, olength;
	size_t clength;

	unsigned int level;

	CompressionType type;
};

BOOST_CLASS_VERSION(Block, 0)

std::ostream &operator<< (std::ostream& os, const Block& rBl);

#endif

