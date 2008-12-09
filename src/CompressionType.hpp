#ifndef COMPRESSIONTYPE_HPP
#define COMPRESSIONTYPE_HPP

#include <string>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include <iostream>

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

	CompressionType() :
#ifdef HAVE_LIBZ
			m_Type (ZLIB)
#elif HAVE_LIBLZO2
			m_Type(LZO)
#elif HAVE_LIBBZ2
			m_Type(BZIP2)
#elif HAVE_LIBLZMA
			m_Type(LZMA)
#else
			m_Type(NONE)
#endif
	{ }

	CompressionType(unsigned char type) :
		m_Type(type)
	{
		// These asserts checks programming error. If
		// some compression method is not supported no
		// code shall call this type of constructor
		// with that unsupported compression type.

#ifndef HAVE_LIBZ
		assert(type != ZLIB);
#endif
#ifndef HAVE_LIBLZO2
		assert(type != LZO);
#endif
#ifndef HAVE_LIBBZ2
		assert(type != BZIP2);
#endif
#ifndef HAVE_LIBLZMA
		assert(type != LZMA);
#endif
	}

	CompressionType(const CompressionType& src) : m_Type (src.m_Type) { }

	bool parseType(std::string type);

	template<typename Mode>
	void push(io::filtering_stream<Mode>& fs) const;

	CompressionType& operator=(const CompressionType& src)
	{
		m_Type = src.m_Type;

		return *this;
	}

	bool operator==(const CompressionType& t)
	{
		return (m_Type == t.m_Type);
	}

	// Minimal size:
	//
	// 1. byte : boost's serialization version (=0 => 1 byte)
	// 2. byte : class version (=0 => 1 byte)
	// 3. byte : compression type (=0 => 1 byte)

	static const int MinSize = 3;

	// Current maximal size
	//
	// 1. byte     : boost's serialization version (=0 => 1 byte)
	// 2. byte     : class version (=0 => 1 byte)
	// 3., 4. byte : compression type (>0 => 2 bytes)

	static const int MaxSize = 4;

	friend std::ostream& operator<<(std::ostream& os, const CompressionType& rObj);
	static void printAllSupportedMethods(std::ostream& os);
};

BOOST_CLASS_VERSION(CompressionType, 0)

#endif

