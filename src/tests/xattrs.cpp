#include "FileRememberXattrs.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rlog/rlog.h>

#include "CompressedMagic.hpp"
#include "CompressionType.hpp"

bool            g_DebugMode = true;
unsigned int	g_BufferedMemorySize;
CompressedMagic g_CompressedMagic;
CompressionType g_CompressionType;
std::string     g_dirLower;
std::string     g_dirMount;
rlog::RLog     *g_RLog;

int main(int argc, char **argv)
{
	g_RLog = new rlog::RLog("xattrs", g_DebugMode ? LOG_DEBUG : LOG_INFO, g_DebugMode);

	int fd_s = open(argv[1], O_RDONLY);
	int fd_d = open(argv[2], O_WRONLY);
	FileRememberXattrs xattrs;
	xattrs.read(fd_s);
	xattrs.write(fd_d);
	return 0;
}

