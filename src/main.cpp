#include "config.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "CompressedMagic.hpp"
#include "FuseCompress.hpp"
#include "TransformTable.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include <rlog/rlog.h>
#include <rlog/StdioNode.h>
#include <rlog/SyslogNode.h>
#include <rlog/RLogChannel.h>
#include <rlog/RLogNode.h>

using namespace std;
using namespace rlog;

namespace po = boost::program_options;

TransformTable	g_TransformTable;
bool            g_DebugMode;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;

static void init_log(void)
{
	if (g_DebugMode)
	{
		static StdioNode log(STDERR_FILENO);
		log.subscribeTo(GetGlobalChannel("warning"));
		log.subscribeTo(GetGlobalChannel("error"));
		log.subscribeTo(GetGlobalChannel("debug"));
	}
	else
	{
		static SyslogNode log(PACKAGE_NAME);
		log.subscribeTo(GetGlobalChannel("warning"));
		log.subscribeTo(GetGlobalChannel("error"));
	}
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
	cout << "Files with any of the following mime type will not be compressed:" << endl << endl;
	cout << g_CompressedMagic << endl;
}

int main(int argc, char **argv)
{
	g_BufferedMemorySize = 100;
	g_DebugMode = false;

	string compressorName = "gz";
	string commandLineOptions;
	string dirLower;
	string dirMount;

	vector<string> fuseOptions;
	fuseOptions.push_back(argv[0]);

	po::options_description desc("Usage: " PACKAGE " [options] dir_lower dir_mount\n" "\nAllowed options");
	desc.add_options()
		("options,o", po::value<string>(&commandLineOptions),
				"fc_c:arg          - set compression method\n"
				"                    (lzo/bz2/gz/none)\n"
				"                    (default: gz)\n"
				"fc_b:arg          - set size of blocks in kilobytes\n"
				"                    (default: 100)\n"
				"fc_d              - run in debug mode\n"
				"fc_ma:\"arg1;arg2\" - files with passed mime types to be\n"
				"                    always not compressed\n"
				"fc_mr:\"arg1;arg2\" - files with passed mime types to be\n"
				"                    always compressed\n"
				"\nOther options are passed directly to fuse library. See fuse documentation for full list of supported options.\n")
		("dir_lower", po::value<string>(&dirLower), "storage directory")
		("dir_mount", po::value<string>(&dirMount), "mount point")
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
					compressorName = *value;
				}
				if (*key == "fc_b")
				{
					g_BufferedMemorySize = boost::lexical_cast<unsigned int>(*value);
				}
				if (*key == "fc_d")
				{
					g_DebugMode = true;
				}
				if (*key == "fc_ma")
				{
					g_CompressedMagic.Add(*value);
				}
				if (*key == "fc_mr")
				{
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
		dirLower = vm["dir_lower"].as<string>();
	}
	else
	{
		print_help(desc);
		exit(EXIT_FAILURE);
	}
	if (vm.count("dir_mount"))
	{
		dirMount = vm["dir_mount"].as<string>();
	}
	else
	{
		print_help(desc);
		exit(EXIT_FAILURE);
	}

	g_BufferedMemorySize *= 1024;
	
	// Set up default options for fuse.
	// 
	// Fuse problems:
	// 	kernel_cache - causes sigfaults when trying to run compiled
	// 	               executables from FuseCompressed filesystem
	//
	fuseOptions.push_back("-o");
	fuseOptions.push_back("default_permissions,use_ino,kernel_cache");
	fuseOptions.push_back(dirMount);

	// Set default transformation as user wanted.
	// 
	if (g_TransformTable.setDefault(compressorName) == false)
	{
		cerr << "Compressor " << compressorName << " not found!" << endl;
		exit(EXIT_FAILURE);
	}
	
	DIR *dir;
	if ((dir = opendir(dirLower.c_str())) == NULL)
	{
		int errns = errno;

		cerr << "Failed to open storage directory "
		     << "'" << dirLower << "': " << strerror(errns) << endl;
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

