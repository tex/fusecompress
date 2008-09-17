bin_PROGRAMS = fusecompress fusecompress_offline

common = \
	boost/iostreams/filter/lzma.cpp \
	CompressionType.cpp \
	FileHeader.cpp \
	CompressedMagic.cpp \
	FileRememberTimes.cpp \
	FuseCompress.cpp \
	File.cpp \
	FileUtils.cpp \
	Compress.cpp \
	Memory.cpp \
	FileManager.cpp \
	FileRawCompressed.cpp \
	FileRawNormal.cpp \
	Block.cpp \
	LayerMap.cpp \
	LinearMap.cpp

noinst_HEADERS = \
	boost/iostreams/filter/lzma.hpp \
	CompressionType.hpp \
	CompressedMagic.hpp \
	FileRememberTimes.hpp \
	Mutex.hpp \
	FileRaw.hpp \
	FuseCompress.hpp \
	File.hpp \
	FileUtils.hpp \
	Compress.hpp \
	Memory.hpp \
	FileManager.hpp \
	FileHeader.hpp \
	FileRawCompressed.hpp \
	FileRawNormal.hpp \
	Block.hpp \
	LinearMap.hpp \
	LayerMap.hpp \
	Lock.hpp

fusecompress_SOURCES = $(common) main.cpp
fusecompress_offline_SOURCES = $(common) main_offline.cpp

AM_CXXFLAGS = -DRLOG_COMPONENT="fusecompress"

AM_CPPFLAGS = \
	-D_GNU_SOURCE \
	-D_REENTRANT \
	-D_POSIX_C_SOURCE=200112L \
	-D_POSIX_SOURCE \
	-D_SVID_SOURCE \
	-D_BSD_SOURCE \
	-D_XOPEN_SOURCE=500 \
	-pedantic -Wno-long-long \
	-Wall

dist_man_MANS = fusecompress.1 fusecompress_offline.1

