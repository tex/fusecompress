bin_PROGRAMS = print_compress

print_compress_SOURCES = print_compress.cpp
print_compress_LDADD =../lzma.o \
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
	../LinearMap.o \
	$(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) $(FUSE_LIBS)

AM_CXXFLAGS = $(BOOST_CXXFLAGS)

AM_LDFLAGS=$(BOOST_LDFLAGS)

AM_CPPFLAGS = \
	-D_GNU_SOURCE \
	-D_REENTRANT \
	-D_POSIX_C_SOURCE=200112L \
	-D_POSIX_SOURCE \
	-D_SVID_SOURCE \
	-D_BSD_SOURCE \
	-D_XOPEN_SOURCE=500 \
	-Wno-long-long \
	-Wall \
	-I.. \
	-fpermissive

