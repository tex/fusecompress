#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/lzo.hpp>
#include <boost/iostreams/filter/xor.hpp>
#include <boost/iostreams/traits.hpp>

#include "CompressionType.hpp"

#include <iostream>

template<>
void CompressionType::push(io::filtering_stream<io::output>& fs)
{
	switch (m_Type) {
	case NONE:
		break;
	case ZLIB:
		fs.push(io::zlib_compressor(io::zlib_params(9, io::zlib::deflated, 15, 8, io::zlib::default_strategy, true)));
		break;
	case BZIP2:
		fs.push(io::bzip2_compressor());
		break;
	case XOR:
		fs.push(io::xor_filter('2'));
		break;
	case LZO:
		fs.push(io::lzo_compressor());
		break;
	case LZMA:
		fs.push(io::lzma_compressor());
		break;
	default:
		break;
	}
}
template<>
void CompressionType::push(io::filtering_stream<io::input>& fs)
{
	switch (m_Type) {
	case NONE:
		break;
	case ZLIB:
		fs.push(io::zlib_decompressor(io::zlib_params(9, io::zlib::deflated, 15, 8, io::zlib::default_strategy, true)));
		break;
	case BZIP2:
		fs.push(io::bzip2_decompressor());
		break;
	case XOR:
		fs.push(io::xor_filter('2'));
		break;
	case LZO:
		fs.push(io::lzo_decompressor());
		break;
	case LZMA:
		fs.push(io::lzma_decompressor());
		break;
	default:
		break;
	}
}

bool CompressionType::parseType(std::string type)
{
	if (type == "none")
		m_Type = NONE;
	else if (type == "zlib")
		m_Type = ZLIB;
	else if (type == "bzip2")
		m_Type = BZIP2;
	else if (type == "xor")
		m_Type = XOR;
	else if (type == "lzo")
		m_Type = LZO;
	else if (type == "lzma")
		m_Type = LZMA;
	else
		return false;

	return true;
}

