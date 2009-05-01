#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(FuseCompress, 2.0, milan.svoboda@centrum.cz)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([.])

AC_LANG_CPLUSPLUS
AC_PREFIX_DEFAULT([/usr/local])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP

# Need to include any user specified flags in the tests below, as they might
# specify required include directories..
FUSEFLAGS="-D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26"
CPPFLAGS="$CPPFLAGS $USER_INCLUDES $FUSEFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS $USER_INCLUDES"
LDFLAGS="$LDFLAGS $PTHREAD_LIBS $USER_LDFLAGS"

AX_BOOST_BASE([1.33.1])
AX_BOOST_SYSTEM
AX_BOOST_SERIALIZATION
AX_BOOST_IOSTREAMS
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_FILESYSTEM

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_mutex_lock],,
    [AC_MSG_ERROR([Can't find pthread.a])],)
AC_CHECK_LIB([magic], [magic_open],,
    [AC_MSG_ERROR([FuseCompress depends on libmagic.])],)

# This function helps with configuring optional libraries.
AC_DEFUN([AX_CHECK_OPTIONAL_LIB],
[
	AC_ARG_WITH([$1],
	AS_HELP_STRING([--with-$1@<:@=DIR@:>@],
		[use the $1 library - it is possible to specify a path for the library]),
	[
	if test "$withval" = "no"; then
		want_lib="no"
	elif test "$withval" = "yes"; then
		want_lib="yes"
		ax_user_lib_path=""
	else
		want_lib="yes"
		ax_user_lib_path="$withval"
	fi
	],
	[want_lib="yes"]
	)

	if test "x$want_lib" = "xyes"; then
		if test "$ax_user_lib_path" != ""; then
			LDFLAGS="$LDFLAGS -L$ax_user_lib_path"
			export LDFLAGS
		fi
		AC_CHECK_LIB([$1], [$2],,,)
	fi
]
)

# Check optional compression libraries.
AX_CHECK_OPTIONAL_LIB([lzma], [lzma_code])
AX_CHECK_OPTIONAL_LIB([z], [uncompress])
AX_CHECK_OPTIONAL_LIB([bz2], [BZ2_bzBuffToBuffCompress])
AX_CHECK_OPTIONAL_LIB([lzo2], [lzo1x_1_compress])

AC_CHECK_HEADER(attr/xattr.h,
    [AC_DEFINE([HAVE_ATTR_XATTR_H], [], ["Have xattr.h header in attr/"])],
    [AC_CHECK_HEADER(sys/xattr.h,
            [AC_DEFINE([HAVE_SYS_XATTR_H], [], ["Have xattr.h header in sys/"])],
	    [AC_MSG_ERROR([Can't find xattr.h in attr/ or /sys])])])

PKG_CHECK_MODULES([FUSE], [fuse])

# Check for a supported FUSE_MAJOR_VERSION.
AC_MSG_CHECKING([For supported FUSE API version])
AC_RUN_IFELSE([ 
    AC_LANG_PROGRAM([[#include <fuse.h>]],
[[
    if(FUSE_MAJOR_VERSION == 2 && FUSE_MINOR_VERSION >= 6)
	return 0;
    else
	return -1;
]])], 
    [AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])
    AC_MSG_FAILURE([FuseCompress requires FUSE 2.6 or newer.])
    ]
)

AC_ARG_ENABLE(profile,
    AC_HELP_STRING([--enable-profile],
                   [collects profile for gprof]),
    CPPFLAGS="$CPPFLAGS -pg" LDFLAGS="$LDFLAGS -pg",)

AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug],
                   [creates debug build]),
    CPPFLAGS="$CPPFLAGS -D_GLIBCXX_DEBUG", CPPFLAGS="$CPPFLAGS -DNDEBUG")

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([fchdir fdatasync ftruncate lchown memmove memset mkdir rmdir strchr strerror utime])

# Tell Makefile.am to process following subdirectories
subdirs="src src/tests"
AC_SUBST(subdirs)

AC_CONFIG_FILES([Makefile] [src/Makefile] [src/tests/Makefile])
AC_OUTPUT
