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

#include "Compress.hpp"
#include "CompressedMagic.hpp"
#include "CompressionType.hpp"
#include "FileRememberXattrs.hpp"

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

namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool            g_DebugMode = false;
bool		g_QuietMode = false;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;
CompressionType g_CompressionType;
std::string     g_dirLower;
std::string     g_dirMount;
rlog::RLog     *g_RLog;

volatile sig_atomic_t    g_BreakFlag = 0;
bool                     g_RawOutput = true;

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
		rError("File (%s) cannot be opened! (%s)", i, strerror(errno));
		return false;
	}

	if (input.isCompressed() == false)
	{
		rInfo(" Not compressed");

		if (g_RawOutput)
		{
			rInfo(" Skipped");

			// Input file is not compressed an user wants
			// to decomrpess file. Return now.

			input.release(i);
			return false;
		}
	}
	else
	{
		rInfo(" Compressed");

		if (g_RawOutput)
		{
		}
		else
		{
			if (input.isCompressedOnlyWith(g_CompressionType))
			{
				rInfo(" All blocks compressed with the same compression method");

				// Input file is compressed only with
				// the same compression type as user wants.
				// Return now.

				input.release(i);
				return false;
			}
			rInfo(" Some block(s) compressed with different compression method than others");
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
		rError("File (%s) cannot be opened! (%s)", o, strerror(errno));
		input.release(i);
		return false;
	}

	// Get the apparent input file size.

	struct stat st;
	int r = input.getattr(i, &st);
	if (r == -1)
	{
		rError("Cannot determine apparent size of input file (%s) (%s)", i, strerror(errno));
		input.release(i);
		return false;
	}

	rInfo(" Processing");

	for (off_t off = 0; off < st.st_size; off += g_BufferedMemorySize)
	{
		if (g_BreakFlag)
		{
			rWarning("Interrupted when processing file (%s)", i);
			rWarning("File is left untouched");
			input.release(i);
			output.release(o);
			return false;
		}
		off_t r = input.read(buffer.get(), g_BufferedMemorySize, off);
		if (r == -1)
		{
			rError("Read failed! (offset: %lld, size: %lld)", (unsigned long long) off ,(unsigned long long) g_BufferedMemorySize);
			input.release(i);
			output.release(o);
			return false;
		}
		off_t rr = output.write(buffer.get(), r, off);
		if (rr != r)
		{
			rError("Write failed! (offset: %lld, size: %lld)", (unsigned long long) off , (unsigned long long) r);
			input.release(i);
			output.release(o);
			return false;
		}
	}

	// Remember extended attributes.

	FileRememberXattrs xattrs;
	xattrs.read(i_fd);
	xattrs.write(o_fd);

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
	
	rInfo("Processing file (%s)", input.string().c_str());

	// Remember times of the input file.

	struct stat stbuf;
	lstat(input.string().c_str(), &stbuf);

	int o_fd = mkstemp(const_cast<char *>(output.string().c_str()));
	if (o_fd < 0)
	{
		rError("Failed to create an temporary file in (%s) directory", input_directory.string().c_str());
		return -1;
	}

	rInfo("Temporary file (%s)", output.string().c_str());

	struct stat o_st;
	if (fstat(o_fd, &o_st) == -1)
	{
		rError("Failed to read stat of temporary file (%s)", output.string().c_str());
		return -1;
	}

	if (!copy(i, output.string().c_str(), i_st, &o_st))
	{
		unlink(output.string().c_str());
		return -1;
	}

	if (rename(output.string().c_str(), i) == -1)
	{
		rError("Failed to rename temporary file (%s) to (%s) (%s)", output.string().c_str(), input.string().c_str(), strerror(errno));
		return -1;
	}

	if (chmod(i, i_st->st_mode) == -1)
	{
		rError("Failed to change mode (%s) (%s)", input.string().c_str(), strerror(errno));
		return -1;
	}

	// Write back original times.

	struct timespec times[2];
	times[0].tv_sec = stbuf.st_atime;
	times[0].tv_nsec = stbuf.st_atim.tv_nsec;
	times[1].tv_sec = stbuf.st_mtime;
	times[1].tv_nsec = stbuf.st_mtim.tv_nsec;
	utimensat(AT_FDCWD, i, times, AT_SYMLINK_NOFOLLOW);
	
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
	std::cout << rDesc << std::endl;
	std::cout << "Supported compression methods:" << std::endl;
	std::cout << "\t"; CompressionType::printAllSupportedMethods(std::cout); std::cout << std::endl << std::endl;
	std::cout << "Files with any of the following mime type will not be compressed:" << std::endl;
	std::cout << g_CompressedMagic << std::endl;
}

int main(int argc, char **argv)
{
	g_BufferedMemorySize = 100;

	string compressorName;
	string commandLineOptions;

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
		("dir_lower", po::value<string>(&g_dirLower), "storage directory")
		("help,h", "print this help")
		("version,v", "print version")
		("quiet,q", "quiet mode")
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
	if (vm.count("quiet"))
	{
		g_QuietMode = true;
	}

	g_RLog = new rlog::RLog("FuseCompress_offline", g_QuietMode ? LOG_NOTICE : LOG_INFO, true);

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
						rError("Compression type not set!");
						exit(EXIT_FAILURE);
					}
					compressorName = *value;
				}
				if (*key == "fc_b")
				{
					if (value == tokens.end())
					{
						rError("Block size not set!");
						exit(EXIT_FAILURE);
					}
					g_BufferedMemorySize = boost::lexical_cast<unsigned int>(*value);
				}
				if (*key == "fc_d")
				{
					g_DebugMode = true;
					g_RLog->setLevel(LOG_DEBUG);
				}
				if (*key == "fc_ma")
				{
					if (value == tokens.end())
					{
						rError("Mime type(s) not set!");
						exit(EXIT_FAILURE);
					}
					g_CompressedMagic.Add(*value);
				}
				if (*key == "fc_mr")
				{
					if (value == tokens.end())
					{
						rError("Mime type(s) not set!");
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
			rError("Compressor %s not found!", compressorName.c_str());
			exit(EXIT_FAILURE);
		}
	}

	fs::path pathLower(g_dirLower, fs::native);

	if (!pathLower.is_complete())
	{
		char cwd[PATH_MAX];

		// Transform relative path to absolute path.

		if (getcwd(cwd, sizeof(cwd)) == NULL)
		{
			rError("Cannot determine current working directory!");
			exit(EXIT_FAILURE);
		}

		pathLower = fs::path(cwd, fs::native) / pathLower;
	}

	// Set signal handler to catch SIGINT (CTRL+C).

	struct sigaction setup_kill;
	memset(&setup_kill, 0, sizeof(setup_kill));
	setup_kill.sa_handler = catch_kill;
	sigaction(SIGINT, &setup_kill, NULL);

	// Iterate over directory structure and execute compress
	// for every files there.

	if (nftw(const_cast<char *>(pathLower.string().c_str()), compress, 100, FTW_PHYS | FTW_CHDIR))
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

