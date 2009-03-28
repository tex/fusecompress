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
	Block.cpp \
	LayerMap.cpp \
	LinearMap.cpp

noinst_HEADERS = \
	assert.h \
	boost/iostreams/device/nonclosable_file_descriptor.hpp \
	boost/iostreams/filter/lzma.hpp \
	boost/iostreams/filter/lzo.hpp \
	boost/iostreams/filter/xor.hpp \
	boost/archive/portable_binary_oarchive.hpp \
	boost/archive/portable_binary_iarchive.hpp \
	boost/integer/cover_operators.hpp \
	boost/integer/endian.hpp \
	boost/math/fpclassify.hpp \
	boost/math/nonfinite_num_facets.hpp \
	boost/math/signbit.hpp \
	boost/math/detail/fp_traits.hpp \
	CompressionType.hpp \
	CompressedMagic.hpp \
	FileRememberTimes.hpp \
	Mutex.hpp \
	FuseCompress.hpp \
	File.hpp \
	FileUtils.hpp \
	Compress.hpp \
	Memory.hpp \
	FileManager.hpp \
	FileHeader.hpp \
	Block.hpp \
	LinearMap.hpp \
	LayerMap.hpp \
	Lock.hpp

fusecompress_SOURCES = $(common) main.cpp
fusecompress_LDADD = $(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB)
fusecompress_offline_SOURCES = $(common) main_offline.cpp
fusecompress_offline_LDADD = $(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB)

AM_CXXFLAGS = $(BOOST_CXXFLAGS) -DRLOG_COMPONENT="fusecompress"

AM_LDFLAGS=$(BOOST_LDFLAGS)

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
	-fpermissive

dist_man_MANS = fusecompress.1 fusecompress_offline.1

