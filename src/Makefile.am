bin_PROGRAMS = fusecompress fusecompress_offline

pkglib_LTLIBRARIES = libfusecompress.la

common = \
	boost/iostreams/filter/lzma.cpp \
	CompressionType.cpp \
	FileHeader.cpp \
	CompressedMagic.cpp \
	FileRememberTimes.cpp \
	FileRememberXattrs.cpp \
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
	boost/integer/endian.hpp

include_HEADERS = \
	CompressionType.hpp \
	CompressedMagic.hpp \
	FileRememberTimes.hpp \
	FileRememberXattrs.hpp \
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

includedir = $(prefix)/include/fusecompress

fusecompress_SOURCES = main.cpp
fusecompress_LDADD = $(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB) $(FUSE_LIBS) libfusecompress.la
fusecompress_offline_SOURCES = main_offline.cpp
fusecompress_offline_LDADD = $(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) $(FUSE_LIBS) libfusecompress.la

libfusecompress_la_SOURCES = $(common)
libfusecompress_la_LDFLAGS = -version-info 8:4:6 -no-undefined

AM_CXXFLAGS=$(BOOST_CXXFLAGS)

AM_LDFLAGS=$(BOOST_LDFLAGS)

AM_CPPFLAGS = \
	-D_GNU_SOURCE \
	-D_REENTRANT \
	-D_POSIX_C_SOURCE=200112L \
	-D_POSIX_SOURCE \
	-D_DEFAULT_SOURCE \
	-D_XOPEN_SOURCE=500 \
	-Wno-long-long \
	-Wall \
	-fpermissive

dist_man_MANS = fusecompress.1 fusecompress_offline.1

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
               $(LN_S) -f fusecompress mount.fusecompress

uninstall-hook:
	cd $(DESTDIR)$(bindir) && \
		rm mount.fusecompress

