#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/lzo.hpp>
#include <boost/iostreams/filter/xor.hpp>
#include <boost/iostreams/traits.hpp>

#include "CompressionType.hpp"
#include "config.h"

#include <iostream>

void CompressionType::printAllSupportedMethods(std::ostream& os)
{
	os << "none, ";
#ifdef HAVE_LIBZ
	os << "zlib, ";
#endif
#ifdef HAVE_LIBLZO2
	os << "lzo, ";
#endif
#ifdef HAVE_LIBBZ2
	os << "bzip2, ";
#endif
#ifdef HAVE_LIBLZMA
	os << "lzma, ";
#endif
	os << "xor";
}

std::ostream& operator<<(std::ostream& os, const CompressionType& rObj)
{
	std::string name;

	switch (rObj.m_Type) {
	case CompressionType::NONE:
		name = "none";
		break;
	case CompressionType::XOR:
		name = "xor";
		break;
	case CompressionType::ZLIB:
		name = "zlib";
		break;
	case CompressionType::BZIP2:
		name = "bzip2";
		break;
	case CompressionType::LZO:
		name = "lzo";
		break;
	case CompressionType::LZMA:
		name = "lzma";
		break;
	}
	return os << name;
}

template<>
void CompressionType::push(io::filtering_stream<io::output>& fs) const
{
	switch (m_Type) {
	case NONE:
		break;
#ifdef HAVE_LIBZ
	case ZLIB:
		fs.push(io::zlib_compressor(io::zlib_params(9, io::zlib::deflated, 15, 8, io::zlib::default_strategy, true)));
		break;
#endif
#ifdef HAVE_LIBBZ2
	case BZIP2:
		fs.push(io::bzip2_compressor());
		break;
#endif
	case XOR:
		fs.push(io::xor_filter('2'));
		break;
#ifdef HAVE_LIBLZO2
	case LZO:
		fs.push(io::lzo_compressor());
		break;
#endif
#ifdef HAVE_LIBLZMA
	case LZMA:
		fs.push(io::lzma_compressor());
		break;
#endif
	default:
	{
		// This shall never happen on OUTPUT filtering stream.

		assert(false);
		break;
	}
	}
}

template<>
void CompressionType::push(io::filtering_stream<io::input>& fs) const
{
	switch (m_Type) {
	case NONE:
		break;
#ifdef HAVE_LIBZ
	case ZLIB:
		fs.push(io::zlib_decompressor(io::zlib_params(9, io::zlib::deflated, 15, 8, io::zlib::default_strategy, true)));
		break;
#endif
#ifdef HAVE_LIBBZ2
	case BZIP2:
		fs.push(io::bzip2_decompressor());
		break;
#endif
	case XOR:
		fs.push(io::xor_filter('2'));
		break;
#ifdef HAVE_LIBLZO2
	case LZO:
		fs.push(io::lzo_decompressor());
		break;
#endif
#ifdef HAVE_LIBLZMA
	case LZMA:
		fs.push(io::lzma_decompressor());
		break;
#endif
	default:
	{
		// If input file is using unsopported version
		// throw an exception and FuseCompress will
		// process such file as uncompressed.

		throw BOOST_IOSTREAMS_FAILURE("unsupported compression type");
		break;
	}
	}
}

bool CompressionType::parseType(std::string type)
{
	if (type == "none")
		m_Type = NONE;
#ifdef HAVE_LIBZ
	else if (type == "zlib")
		m_Type = ZLIB;
#endif
#ifdef HAVE_LIBBZ2
	else if (type == "bzip2")
		m_Type = BZIP2;
#endif
	else if (type == "xor")
		m_Type = XOR;
#ifdef HAVE_LIBLZO2
	else if (type == "lzo")
		m_Type = LZO;
#endif
#ifdef HAVE_LIBLZMA
	else if (type == "lzma")
		m_Type = LZMA;
#endif
	else
		return false;

	return true;
}

