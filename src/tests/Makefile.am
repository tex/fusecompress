bin_PROGRAMS = print_compress

print_compress_SOURCES = print_compress.cpp

LDADD = \
	../lzma.o \
	../CompressionType.o \
	../FileHeader.o \
	../CompressedMagic.o \
	../FileRememberTimes.o \
	../FuseCompress.o \
	../File.o \
	../FileUtils.o \
	../Compress.o \
	../Memory.o \
	../FileManager.o \
	../Block.o \
	../LayerMap.o \
	../LinearMap.o

AM_CXXFLAGS = $(BOOST_CXXFLAGS) -DRLOG_COMPONENT="fusecompress"

AM_LDFLAGS=$(BOOST_LDFLAGS) $(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB)

AM_CPPFLAGS = \
	-D_GNU_SOURCE \
	-D_REENTRANT \
	-D_POSIX_C_SOURCE=200112L \
	-D_POSIX_SOURCE \
	-D_SVID_SOURCE \
	-D_BSD_SOURCE \
	-D_XOPEN_SOURCE=500 \
	-pedantic -Wno-long-long \
	-Wall \
	-I..

