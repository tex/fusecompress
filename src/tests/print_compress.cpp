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

#include "rlog/rlog.h"

#include "Compress.hpp"
#include "CompressedMagic.hpp"
#include "CompressionType.hpp"
#include "FileManager.hpp"

#include <boost/version.hpp>
#if BOOST_VERSION >= 105600
#define BOOST_DISABLE_ASSERTS
#endif

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

bool            g_DebugMode = true;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;
CompressionType g_CompressionType;
std::string     g_dirLower;
std::string     g_dirMount;
rlog::RLog     *g_RLog;

bool                     g_RawOutput = true;

void init_log(void)
{
	g_RLog = new rlog::RLog("print_compress", g_DebugMode ? LOG_DEBUG : LOG_INFO, g_DebugMode);
}

int main(int argc, char **argv)
{
	g_BufferedMemorySize = 100 * 1024;

	string filename;

	po::options_description desc;
	desc.add_options()
		("filename", po::value<string>(&filename), "filename")
	;

	po::positional_options_description pdesc;
	pdesc.add("filename", 1);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(pdesc).run(), vm);
	} catch (...) {
		exit(EXIT_FAILURE);
	}
	po::notify(vm);

	if (!vm.count("filename"))
	{
		exit(EXIT_FAILURE);
	}

	std::cout << "Processing: " << filename << std::endl;

	init_log();

	struct stat filename_stat;
	stat(filename.c_str(), &filename_stat);	
	Compress input(&filename_stat, filename.c_str());
	input.open(filename.c_str(), 0);

	std::cout << "isCompressed: " << input.isCompressed() << std::endl;
	std::cout << input;

	exit(EXIT_SUCCESS);
}

