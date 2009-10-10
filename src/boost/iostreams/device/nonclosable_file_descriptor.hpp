// (C) Copyright Milan Svoboda 2009.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef NONCLOSABLE_FILE_DESCRIPTOR_HPP
#define NONCLOSABLE_FILE_DESCRIPTOR_HPP

#include <boost/iostreams/device/file_descriptor.hpp>

namespace boost { namespace iostreams {

class BOOST_IOSTREAMS_DECL nonclosable_file_descriptor : public file_descriptor {
public:
    struct category
        : seekable_device_tag
        { };

    nonclosable_file_descriptor() : file_descriptor() { }
    explicit nonclosable_file_descriptor(int fd)
        : file_descriptor(fd, false)
        { }
#ifdef BOOST_IOSTREAMS_WINDOWS
    explicit nonclosable_file_descriptor(handle_type handle)
        : file_descriptor(handle, false)
        { }
#endif
    explicit nonclosable_file_descriptor( const std::string& path,
                              BOOST_IOS::openmode mode =
                                  BOOST_IOS::in | BOOST_IOS::out,
                              BOOST_IOS::openmode base_mode =
                                  BOOST_IOS::in | BOOST_IOS::out )
        : file_descriptor(path, mode, base_mode) {}
};

} }

#endif

