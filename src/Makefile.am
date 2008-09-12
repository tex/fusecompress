bin_PROGRAMS = fusecompress fusecompress_offline

common = \
	CompressedMagic.cpp \
	FileRememberTimes.cpp \
	FuseCompress.cpp \
	File.cpp \
	FileUtils.cpp \
	Compress.cpp \
	Memory.cpp \
	FileManager.cpp \
	FileHeader.cpp \
	FileRawCompressed.cpp \
	FileRawNormal.cpp \
	Block.cpp \
	LayerMap.cpp \
	LinearMap.cpp \
	TransformTable.cpp \
	TransformNONE.cpp \
	TransformXOR.cpp \
	TransformCompress.cpp \
	TransformCompressGZ.cpp \
	TransformCompressBZ2.cpp \
	TransformCompressLZO.cpp \
	minilzo/minilzo.c

noinst_HEADERS = \
	CompressedMagic.hpp \
	FileRememberTimes.hpp \
	ByteOrder.hpp \
	Mutex.hpp \
	FileRaw.hpp \
	Transform.hpp \
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
	TransformTable.hpp \
	TransformNONE.hpp \
	TransformXOR.hpp \
	TransformCompress.hpp \
	TransformCompressGZ.hpp \
	TransformCompressBZ2.hpp \
	TransformCompressLZO.hpp \
	minilzo/minilzo.h \
	minilzo/lzoconf.h \
	minilzo/lzodefs.h \
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

