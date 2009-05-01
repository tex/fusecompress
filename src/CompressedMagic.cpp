#include <cassert>
#include <cstring>
#include <cstdlib>
#include <rlog/rlog.h>
#include <stdlib.h>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

#include "Lock.hpp"
#include "CompressedMagic.hpp"

void CompressedMagic::PopulateTable()
{
	m_table.insert("audio/mp4");
	m_table.insert("audio/mpeg");
	m_table.insert("audio/x-pn-realaudio");
	m_table.insert("audio/x-mod");
	m_table.insert("application/ogg");
	m_table.insert("application/pdf");
	m_table.insert("application/vnd.rn-realmedia");
	m_table.insert("application/x-arc");
	m_table.insert("application/x-arj");
	m_table.insert("application/x-bzip2");
	m_table.insert("application/x-compress");
	m_table.insert("application/x-cpio");
	m_table.insert("application/x-debian-package");
	m_table.insert("application/x-gzip");
	m_table.insert("application/x-lharc");
	m_table.insert("application/x-quicktime");
	m_table.insert("application/x-rar");
	m_table.insert("application/x-rpm");
	m_table.insert("application/x-shockwave-flash");
	m_table.insert("application/x-xz");
	m_table.insert("application/x-zip");
	m_table.insert("application/x-zoo");
	m_table.insert("image/gif");
	m_table.insert("image/jpeg");
	m_table.insert("image/jp2");
	m_table.insert("image/png");
	m_table.insert("image/x-quicktime");
	m_table.insert("video/3gpp");
	m_table.insert("video/mp4");
	m_table.insert("video/mp4v-es");
	m_table.insert("video/mpeg");
	m_table.insert("video/mp2t");
	m_table.insert("video/mpv");
	m_table.insert("video/quicktime");
}

CompressedMagic::CompressedMagic()
{
	m_magic = magic_open(MAGIC_MIME_TYPE|MAGIC_PRESERVE_ATIME);
	if (!m_magic)
	{
		rError("CompressedMagic::CompressedMagic magic_open failed with: %s",
				magic_error(m_magic));
		abort();
	}
	magic_load(m_magic, NULL);

	PopulateTable();
}

CompressedMagic::~CompressedMagic()
{
	m_table.clear();

	magic_close(m_magic);
}

std::ostream &operator<<(std::ostream &os, const CompressedMagic &rObj)
{
	CompressedMagic::con_t::const_iterator it;
	unsigned int len = 0;

	// Get length of the longest name.
	//
	for (it  = rObj.m_table.begin(); it != rObj.m_table.end(); ++it)
	{
		if (len < (*it).length())
			len = (*it).length();
	}

	// Compute how many colums fit to a terminal.
	//
	unsigned int cols = (80 - 2) / (len + 5);
	unsigned int col;

	os << "  ";

	for (it  = rObj.m_table.begin(), col = 1; it != rObj.m_table.end(); ++it, col++)
	{
		os << std::setw(len + 5) << std::left << *it;
		if ((col % cols)== 0)
			os << std::endl << "  ";
	}
	return os;
}

void CompressedMagic::Add(const std::string &rMimes)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(";");
	tokenizer tokens(rMimes, sep);
	for (tokenizer::iterator tok_it = tokens.begin(); tok_it != tokens.end(); ++tok_it)
	{
		m_table.insert(boost::algorithm::to_lower_copy(*tok_it));
	}
}

void CompressedMagic::Remove(const std::string &rMimes)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(";");
	tokenizer tokens(rMimes, sep);
	for (tokenizer::iterator tok_it = tokens.begin(); tok_it != tokens.end(); ++tok_it)
	{
		con_t::iterator it = m_table.find(boost::algorithm::to_lower_copy(*tok_it));
		if (it != m_table.end())
			m_table.erase(it);
	}
}

bool CompressedMagic::isNativelyCompressed(const char *buf, int len)
{
	const char *mime;

	Lock lock(m_Mutex);

	mime = magic_buffer(m_magic, buf, len);

	if (mime != NULL)
	{
		rDebug("Data identified as %s", mime);

		if (m_table.find(mime) != m_table.end())
		{
			return true;
		}
	}
	return false;
}

bool CompressedMagic::isNativelyCompressed(const char *name)
{
	const char *mime;

	Lock lock(m_Mutex);

	mime = magic_file(m_magic, name);

	if (mime != NULL)
	{
		rDebug("Data identified as %s", mime);

		if (m_table.find(mime) != m_table.end())
		{
			return true;
		}
	}
	return false;
}

