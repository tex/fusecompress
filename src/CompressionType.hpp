#ifndef COMPRESSIONTYPE_HPP
#define COMPRESSIONTYPE_HPP

#include <string>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

namespace io = boost::iostreams;

class CompressionType
{
	unsigned char m_Type;

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/)
	{
		ar & m_Type;
	}
public:
	enum Method
	{
		NONE	= 0,
		XOR	= 1,
		ZLIB	= 2,
		BZIP2	= 3,
		LZO	= 4,
		LZMA	= 5
	};

	CompressionType() : m_Type (ZLIB) {}
	CompressionType(unsigned char type) : m_Type (type) {}

	bool parseType(std::string type);

	template<typename Mode>
	void push(io::filtering_stream<Mode>& fs);

	// 1. 2 bytes : boost::serialization header
	//    1. byte : ?
	//    2. byte : class version
	// The last byte is unsigned char.

	static const int MaxSize = 3;
};

BOOST_CLASS_VERSION(CompressionType, 0)

#endif
