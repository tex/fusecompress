#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <ftw.h>
#include <cstdlib>
#include <limits.h>

#include <rlog/rlog.h>
#include <rlog/StdioNode.h>
#include <rlog/SyslogNode.h>
#include <rlog/RLogChannel.h>
#include <rlog/RLogNode.h>

#include "Compress.hpp"
#include "CompressedMagic.hpp"
#include "CompressionType.hpp"

#include <boost/scoped_array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include "boost/filesystem/operations.hpp" // includes boost/filesystem/path.hpp
#include "boost/filesystem/fstream.hpp"    // ditto


#include <iostream>
#include <vector>
#include <string>

using namespace std;
using namespace rlog;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool            g_DebugMode;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;
CompressionType g_CompressionType;

volatile sig_atomic_t    g_BreakFlag = 0;
bool                     g_RawOutput = true;

void init_log(void)
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

void catch_kill(int signum)
{
	g_BreakFlag = 1;
}

bool copy(const char *i, const char *o, const struct stat *i_st, const struct stat *o_st)
{
	Compress input(i_st, i);

	int i_fd = input.open(i, O_RDONLY);
	if (i_fd == -1)
	{
		cerr << "File " << i << " cannot be opened!" << endl;
		cerr << " (" << strerror(errno) << ")" << endl;
		return false;
	}

	if (input.isCompressed() == false)
	{
		std::cout << " Not compressed" << std::endl;

		if (g_RawOutput)
		{
			std::cout << " Skipped" << std::endl;

			// Input file is not compressed an user wants
			// to decomrpess file. Return now.
			//
			input.release(i);
			return false;
		}
	}
	else
	{
		std::cout << " Compressed" << std::endl;

		if (g_RawOutput)
		{
		}
		else
		{
			if (input.isCompressedOnlyWith(g_CompressionType))
			{
				std::cout << " All blocks compressed with " << g_CompressionType << std::endl;

				// Input file is compressed only with
				// the same compression type as user wants.
				// Return now.
				//
				input.release(i);
				return false;
			}
			std::cout << " Some block(s) not compressed with " << g_CompressionType << std::endl;
		}
	}

	Compress output(o_st, o);

	if (g_RawOutput)
	{
		output.setCompressed(false);
	}

	boost::scoped_array<char> buffer(new char[g_BufferedMemorySize]);

	int o_fd = output.open(o, O_WRONLY);
	if (o_fd == -1)
	{
		cerr << "File " << o << " cannot be opened!" << endl;
		cerr << " (" << strerror(errno) << ")" << endl;
		input.release(i);
		return false;
	}

	// Get the apparent input file size.

	struct stat st;
	int r = input.getattr(i, &st);
	assert (r != -1);

	std::cout << " Processing";

	for (off_t off = 0; off < st.st_size; off += g_BufferedMemorySize)
	{
		if (g_BreakFlag)
		{
			cerr << std::endl << "Interrupted when processing file " << i << endl;
			cerr << "File is left untouched" << endl;
			input.release(i);
			output.release(o);
			return false;
		}
		off_t r = input.read(buffer.get(), g_BufferedMemorySize, off);
		if (r == -1)
		{
			cerr << std::endl << "Read failed! (offset: " << off << ", size: " << g_BufferedMemorySize << ")" << endl;
			input.release(i);
			output.release(o);
			return false;
		}
		off_t rr = output.write(buffer.get(), r, off);
		if (rr != r)
		{
			cerr << std::endl << "Write failed! (offset: " << off << ", size: " << r << ")" << endl;
			input.release(i);
			output.release(o);
			return false;
		}
		std::cout << ".";
	}

	std::cout << std::endl;

	input.release(i);
	output.release(o);
	return true;
}

int compress(const char *i, const struct stat *i_st, int mode, struct FTW *n)
{
	if (!((mode == FTW_F) && (S_ISREG(i_st->st_mode))))
		return 0;

	fs::path input(i, fs::native);
	fs::path input_directory(input.branch_path());
	fs::path output(input_directory / "XXXXXX");
	
	cout << "Processing file " << input.string() << endl;

	int o_fd = mkstemp(const_cast<char *>(output.string().c_str()));
	if (o_fd < 0)
	{
		std::cerr << "Failed to create an temporary file in (" << input_directory.string() << ") directory" << std::endl;
		return -1;
	}

	cout << "Temporary file " << output.string() << endl;

	struct stat o_st;
	if (fstat(o_fd, &o_st) == -1)
	{
		std::cerr << "Failed to read stat of temporary file (" << output.string() << ")" << std::endl;
		return -1;
	}

	if (!copy(i, output.string().c_str(), i_st, &o_st))
	{
		unlink(output.string().c_str());
		return 0;
	}

	if (rename(output.string().c_str(), i) == -1)
	{
		std::cerr << "Failed to rename temporary file (" << output.string() << ") to (" << input.string() << ")" << std::endl;
		std::cerr << " " << strerror(errno) << std::endl;
		return -1;
	}

	if (chmod(i, i_st->st_mode) == -1)
	{
		std::cerr << "Failed to change mode (" << input.string() << ")" << std::endl;
		std::cerr << " " << strerror(errno) << std::endl;
		return -1;
	}

	return 0;
}

void print_license()
{
	printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Copyright (C) 2007, 2008  Milan Svoboda.\n");
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

	string compressorName;
	string commandLineOptions;
	string dirLower;

	vector<string> fuseOptions;
	fuseOptions.push_back(argv[0]);

	po::options_description desc("Usage: " PACKAGE "_offline [OPTIONS] dir_lower\n"
	                                "\nInput file may also be a directory name. Files in\n"
	                                "specified directory will be processed recursively.\n\n"
	                                "Allowed options");
	desc.add_options()
		("options,o", po::value<string>(&commandLineOptions),
				"fc_c:arg  - set compression method (lzo/bzip2/zlib/lzma/none)\n"
				"            (default: gz)\n"
				"fc_b:arg  - set size of blocks in kilobytes\n"
				"            (default: 100)\n"
				"fc_d      - run in debug mode\n"
				"fc_ma:arg - files with passed mime types to be\n"
				"            always not compressed\n"
				"fc_mr:arg - files with passed mime types to be\n"
				"            always compressed\n"
				"\nOther options are passed directly to fuse library. See fuse documentation for full list of supported options.\n")
		("dir_lower", po::value<string>(&dirLower), "storage directory")
		("help,h", "print this help")
		("version,v", "print version")
	;

	po::positional_options_description pdesc;
	pdesc.add("dir_lower", 1);

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
	if (!vm.count("dir_lower"))
	{
		print_help(desc);
		exit(EXIT_FAILURE);
	}

	g_BufferedMemorySize *= 1024;

	if (compressorName != "")
	{
		g_RawOutput = false;

		if (g_CompressionType.parseType(compressorName) == false)
		{
			cerr << "Compressor " << compressorName << " not found!" << endl;
			exit(EXIT_FAILURE);
		}
	}

	std::cout << dirLower << std::endl;

	fs::path pathLower(dirLower, fs::native);

	if (!pathLower.is_complete())
	{
		char cwd[PATH_MAX];

		// Transform relative path to absolute path.
		//
		if (getcwd(cwd, sizeof(cwd)) == NULL)
		{
			cerr << "Cannot determine current working directory" << endl;
			exit(EXIT_FAILURE);
		}

		pathLower = fs::path(cwd, fs::native) / pathLower;
	}

	init_log();

	// Set signal handler to catch SIGINT (CTRL+C).
	//
	struct sigaction setup_kill;
	memset(&setup_kill, 0, sizeof(setup_kill));
	setup_kill.sa_handler = catch_kill;
	sigaction(SIGINT, &setup_kill, NULL);

	// Iterate over directory structure and execute compress
	// for every files there.
	//
	if (nftw(const_cast<char *>(pathLower.string().c_str()), compress, 100, FTW_PHYS | FTW_CHDIR))
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

