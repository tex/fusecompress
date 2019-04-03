/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    FuseCompress is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FuseCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "CompressedMagic.hpp"
#include "FuseCompress.hpp"
#include "CompressionType.hpp"

#include <boost/version.hpp>
#if BOOST_VERSION >= 105600
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include "rlog/rlog.h"

using namespace std;

namespace po = boost::program_options;

bool            g_DebugMode;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;
CompressionType g_CompressionType;
std::string     g_dirLower;
std::string     g_dirMount;
rlog::RLog     *g_RLog;

static void init_log(void)
{
	g_RLog = new rlog::RLog("FuseCompress", g_DebugMode ? LOG_DEBUG : LOG_INFO, g_DebugMode);
}

void print_license()
{
	printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Copyright (C) 2007,2008  Milan Svoboda.\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

void print_help(const po::options_description &rDesc)
{
	cout << rDesc << endl;
	cout << "Supported compression methods:" << endl;
	cout << "\t"; CompressionType::printAllSupportedMethods(cout); cout << endl << endl;
	cout << "Files with any of the following mime type will not be compressed:" << endl;
	cout << g_CompressedMagic << endl;
}

int main(int argc, char **argv)
{
	g_BufferedMemorySize = 100;
	g_DebugMode = false;

	string compressorName;
	string commandLineOptions;

	vector<string> fuseOptions;
	fuseOptions.push_back(argv[0]);

	po::options_description desc("Usage: " PACKAGE " [options] dir_lower dir_mount\n" "\nAllowed options");
	desc.add_options()
		("options,o", po::value<string>(&commandLineOptions),
				"fc_c:arg          - compression method\n"
				"                    (lzo/bzip2/zlib/lzma)\n"
				"                    (default: zlib)\n"
				"fc_b:arg          - size of blocks in kilobytes\n"
				"                    (default: 100)\n"
				"fc_d              - run in debug mode\n"
				"fc_ma:\"arg1;arg2\" - files with passed mime types to be\n"
				"                    always not compressed\n"
				"fc_mr:\"arg1;arg2\" - files with passed mime types to be\n"
				"                    always compressed\n"
				"\nOther options are passed directly to fuse library. See fuse documentation for full list of supported options.\n")
		("dir_lower", po::value<string>(&g_dirLower), "storage directory")
		("dir_mount", po::value<string>(&g_dirMount), "mount point")
		("help,h", "print this help")
		("version,v", "print version")
	;

	po::positional_options_description pdesc;
	pdesc.add("dir_lower", 1);
	pdesc.add("dir_mount", 1);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(pdesc).run(), vm);
	} catch (...) {
		print_help(desc);
		exit(EXIT_FAILURE);
	}
	po::notify(vm);

	if (vm.count("help"))
	{
		print_help(desc);
		exit(EXIT_SUCCESS);
	}
	if (vm.count("version"))
	{
		print_license();
		exit(EXIT_SUCCESS);
	}
	if (vm.count("options"))
	{
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep(",");
		tokenizer tokens(commandLineOptions, sep);

		for (tokenizer::iterator tok_it = tokens.begin(); tok_it != tokens.end(); ++tok_it)
		{
			if ((*tok_it).find_first_of("fc_", 0, 3) == 0)
			{
				typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
				boost::char_separator<char> sep(":");
				tokenizer tokens(*tok_it, sep);

				tokenizer::iterator key = tokens.begin();
				tokenizer::iterator value = key; ++value;

				if (*key == "fc_c")
				{
					if (value == tokens.end())
					{
						std::cerr << "Compression type not set!" << std::endl;
						exit(EXIT_FAILURE);
					}
					compressorName = *value;
				}
				if (*key == "fc_b")
				{
					if (value == tokens.end())
					{
						std::cerr << "Block size not set!" << std::endl;
						exit(EXIT_FAILURE);
					}
					g_BufferedMemorySize = boost::lexical_cast<unsigned int>(*value);
				}
				if (*key == "fc_d")
				{
					fuseOptions.push_back("-f");
					g_DebugMode = true;
				}
				if (*key == "fc_ma")
				{
					if (value == tokens.end())
					{
						std::cerr << "Mime type(s) not set!" << std::endl;
						exit(EXIT_FAILURE);
					}
					g_CompressedMagic.Add(*value);
				}
				if (*key == "fc_mr")
				{
					if (value == tokens.end())
					{
						std::cerr << "Mime type(s) not set!" << std::endl;
						exit(EXIT_FAILURE);
					}
					g_CompressedMagic.Remove(*value);
				}
			}
			else
			{
				fuseOptions.push_back("-o");
				fuseOptions.push_back(*tok_it);
			}
		}
	}
	if (vm.count("dir_lower"))
	{
		g_dirLower = vm["dir_lower"].as<string>();
	}
	else
	{
		print_help(desc);
		exit(EXIT_FAILURE);
	}
	if (vm.count("dir_mount"))
	{
		g_dirMount = vm["dir_mount"].as<string>();
	}
	else
	{
		print_help(desc);
		exit(EXIT_FAILURE);
	}

	g_BufferedMemorySize *= 1024;
	
	// Set up default options for fuse.
	// 
	fuseOptions.push_back("-o");
	fuseOptions.push_back("default_permissions,use_ino,kernel_cache");
	fuseOptions.push_back(g_dirMount);

	// Set default transformation as user wanted.
	// 
	if ((compressorName != "") &&
	    (g_CompressionType.parseType(compressorName) == false))
	{
		cerr << "Compressor " << compressorName << " not found!" << endl;
		exit(EXIT_FAILURE);
	}
	
	DIR *dir;
	if ((dir = opendir(g_dirLower.c_str())) == NULL)
	{
		int errns = errno;

		cerr << "Failed to open storage directory "
		     << "'" << g_dirLower << "': " << strerror(errns) << endl;
		exit(EXIT_FAILURE);
	}

	vector<const char *> fuse_c_str;
	for (unsigned int i = 0; i < fuseOptions.size(); ++i)
	{
		fuse_c_str.push_back((fuseOptions[i].c_str()));
	}

	init_log();
	FuseCompress fusecompress;

	umask(0);
	return fusecompress.Run(dir, fuse_c_str.size(), &fuse_c_str[0]);
}

