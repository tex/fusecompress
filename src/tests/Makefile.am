bin_PROGRAMS = print_compress xattrs

xattrs_SOURCES = xattrs.cpp
xattrs_LDADD =../libfusecompress.la \
	$(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) $(FUSE_LIBS)


print_compress_SOURCES = print_compress.cpp
print_compress_LDADD =../libfusecompress.la \
	$(BOOST_SERIALIZATION_LIB) $(BOOST_IOSTREAMS_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) $(FUSE_LIBS)

AM_CXXFLAGS = $(BOOST_CXXFLAGS)

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
	-I.. \
	-fpermissive -O0 -g

