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

#include "Compress.hpp"
#include "CompressedMagic.hpp"
#include "CompressionType.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>
#include <vector>
#include <sstream>
#include <string>

using namespace std;

namespace po = boost::program_options;

bool            g_DebugMode;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;
CompressionType g_CompressionType;

volatile sig_atomic_t    g_BreakFlag = 0;
bool                     g_RawOutput = true;

void catch_kill(int signum)
{
	g_BreakFlag = 1;
}

bool prepare(const char *input , struct stat *input_st,
             const char *output, struct stat *output_st)
{
	int r;
	
	r = lstat(input, input_st);
	if (r == -1)
	{
		cerr << "File " << input << " cannot be opened!" << endl;
		cerr << " (" << strerror(errno) << ")" << endl;
		return false;
	}
	
	r = mknod(output, 0600, S_IFREG);
	if (r == -1)
	{
		cerr << "File " << output << " cannot be created!";
		cerr << " (" << strerror(errno) << ")" << endl;
		return false;
	}

	r = lstat(output, output_st);
	if (r == -1)
	{
		cerr << "File " << output << " cannot be opened!";
		cerr << " (" << strerror(errno) << ")" << endl;
		unlink(output);
		return false;
	}
	return true;
}

/**
 * input	- file name of the input file
 * output	- file name of the output file
 */
bool copy(const char *input, struct stat *st, const char *output)
{
	bool		 b = true;
	int		 r;
	int		 rr;
	int		 fd;
	char		*buf = NULL;
	struct stat	 sto;
	Compress	*c = NULL;
	Compress	*o = NULL;

	if (::prepare(input, st, output, &sto) == false)
	{
		return false;
	}

	if (g_RawOutput)
	{
		// Lie about file size to force Compress to
		// use FileRawNormal...
		//
		sto.st_size = 1;
	}

	try {
		buf = new char[g_BufferedMemorySize];

		c = new Compress(st);
		o = new Compress(&sto);
	}
	catch (const bad_alloc e)
	{
		delete[] buf;
		delete   c;
		delete   o;

		// Output filename was created, delete it now to
		// clean up.
		//
		unlink(output);
		
		cerr << "No free memory!" << endl;
		cerr << "Try to use smaller block size." << endl;
		return false;	
	}

	fd = c->open(input, O_RDONLY);
	if (fd == -1)
	{
		cerr << "File " << input << " cannot be opened!" << endl;
		cerr << " (" << strerror(errno) << ")" << endl;
		b = false;
		goto out1;
	}

	fd = o->open(output, O_WRONLY);
	if (fd == -1)
	{
		cerr << "File " << output << " cannot be opened!" << endl;
		cerr << " (" << strerror(errno) << ")" << endl;
		b = false;
		goto out2;
	}

	// Update struct stat of the compressed
	// file with real values of the uncompressed file.
	// 
	r = c->getattr(input, st);
	assert (r != -1);

	for (off_t i = 0; i < st->st_size; i += g_BufferedMemorySize)
	{
		if (g_BreakFlag)
		{
			cerr << "Interrupted when processing file " << input << endl;
			cerr << "File is left untouched" << endl;
			b = false;
			goto out;
		}
		r = c->read(buf, g_BufferedMemorySize, i);
		if (r == -1)
		{
			cerr << "Read failed! (offset: " << i << ", size: " << g_BufferedMemorySize << ")" << endl;
			b = false;
			goto out;
		}
		rr = o->write(buf, r, i);
		if (rr != r)
		{
			cerr << "Write failed! (offset: " << i << ", size: " << r << ")" << endl;
			b = false;
			goto out;
		}
	}
out:	o->release(input);
out2:	c->release(output);
	
out1:	delete c;
	delete o;

	delete[] buf;

	if (!b)
		unlink(output);

	return b;
}

inline bool test(const char *input)
{
/* TODO
	FileHeader	 fh;
	bool             fl;

	fl = fh.restore(input);

	if (g_RawOutput && !fl)
	{
		// Decompress files mode turned on and the
		// file is not compressed with FuseCompress.
		//
		return false;
	}

	if (fl)
	{
		// File is compressed with FuseCompress.
		//
		if (fh.type == g_TransformTable.getDefaultIndex())
		{
			// File is compressed with the same compression
			// method.
			//
			cout << "\tAlready compressed with required method!" << endl;
			return false;
		}
		return true;
	}

	// File is not compressed with FuseCompress.
	//

	if (g_CompressedMagic.isNativelyCompressed(input))
	{
		cout <<"\tFile contains compressed data!" << endl;
		return false;
	}
*/
	return true;
}

int compress(const char *input, const struct stat *st, int mode, struct FTW *n)
{
	if ((mode == FTW_F) && (S_ISREG(st->st_mode)))
	{
		const char	*ext;
		struct stat	 nst;
		stringstream	 tmp;

		cout << "Processing file " << input << endl;

		if (!test(input))
			return 0;
	
		// Create temp filename, make sure that
		// extension is preserved...
		//
		ext = strrchr(input, '/');
		ext = strrchr(ext, '.');

		tmp << input << ".__f_tmp_c__" << ext;

		string out(tmp.str());

		if (copy(input, &nst, out.c_str()) == false)
			return -1;

		if (rename(out.c_str(), input) == -1)
		{
			cerr << "Rename(" << out << ", ";
			cerr <<	input << ") failed with:";
			cerr << strerror(errno) << endl;
			return -1;
		}

		if (chmod(input, nst.st_mode) == -1)
		{
			cerr << "Chmod(" << input << ") failed with:";
			cerr << strerror(errno) << endl;
			return -1;
		}
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
	char  		input[PATH_MAX];
	const char *    pinput = input;

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

	pinput = dirLower.c_str();

	if (pinput[0] != '/')
	{
		// Transform relative path to absolute path.
		//
		if (getcwd(input, sizeof(input)) == NULL)
		{
			cerr << "Cannot determine current working directory" << endl;
			exit(EXIT_FAILURE);
		}
		input[strlen(input)] = '/';
		strcpy(input + strlen(input), pinput);
	}
	else
		strcpy(input, pinput);

	// Set signal handler to catch SIGINT (CTRL+C).
	//
	struct sigaction setup_kill;
	memset(&setup_kill, 0, sizeof(setup_kill));
	setup_kill.sa_handler = catch_kill;
	sigaction(SIGINT, &setup_kill, NULL);

	// Iterate over directory structure and execute compress
	// for every files there.
	//
	if (nftw(input, compress, 100, FTW_PHYS | FTW_CHDIR))
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

