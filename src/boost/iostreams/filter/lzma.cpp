#include "config.h"

// Test for presence of lzma library on the system.
//
// Do it simple this way until I figure out how to
// change the Makefile.am to not build this file when
// lzma library is not present on the system.

#ifdef HAVE_LIBLZMA

// (C) Copyright Jonathan Turkanis 2003.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Define BOOST_IOSTREAMS_SOURCE so that <boost/iostreams/detail/config.hpp> 
// knows that we are building the library (possibly exporting code), rather 
// than using it (possibly importing code).
#define BOOST_IOSTREAMS_SOURCE 

#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/filter/lzma.hpp> 

namespace boost { namespace iostreams {

namespace lzma {

                    // Compression levels

const uint32_t no_compression       = 0;
const uint32_t best_speed           = 1;
const uint32_t best_compression     = 9;
const uint32_t default_compression  = 2;

                    // Status codes

const int okay                 = LZMA_OK;
const int stream_end           = LZMA_STREAM_END;
const int unsupported_check    = LZMA_UNSUPPORTED_CHECK;
const int mem_error            = LZMA_MEM_ERROR;
const int options_error        = LZMA_OPTIONS_ERROR;
const int data_error           = LZMA_DATA_ERROR;
const int buf_error            = LZMA_BUF_ERROR;
const int prog_error           = LZMA_PROG_ERROR;

                    // Flush codes

const lzma_action finish               = LZMA_FINISH;
const lzma_action full_flush           = LZMA_FULL_FLUSH;
const lzma_action sync_flush           = LZMA_SYNC_FLUSH;
const lzma_action run                  = LZMA_RUN;

                    // Code for current OS

} // End namespace lzma. 

//------------------Implementation of lzma_error------------------------------//
                    
lzma_error::lzma_error(int error) 
    : BOOST_IOSTREAMS_FAILURE("lzma error"), error_(error) 
    { }

void lzma_error::check(int error)
{
    switch (error) {
    case LZMA_OK: 
    case LZMA_STREAM_END: 
        return;
    case LZMA_MEM_ERROR: 
        throw std::bad_alloc();
    default:
        throw lzma_error(error);
    }
}

//------------------Implementation of lzma_base-------------------------------//

namespace detail {

lzma_base::lzma_base()
    : stream_(new lzma_stream), calculate_crc_(false), crc_(0)
    { }

lzma_base::~lzma_base() { delete static_cast<lzma_stream*>(stream_); }

void lzma_base::before( const char*& src_begin, const char* src_end,
                        char*& dest_begin, char* dest_end )
{
    lzma_stream* s = static_cast<lzma_stream*>(stream_);
    s->next_in = reinterpret_cast<uint8_t*>(const_cast<char*>(src_begin));
    s->avail_in = static_cast<size_t>(src_end - src_begin);
    s->next_out = reinterpret_cast<uint8_t*>(dest_begin);
    s->avail_out= static_cast<size_t>(dest_end - dest_begin);
}

void lzma_base::after(const char*& src_begin, char*& dest_begin, bool compress)
{
    lzma_stream* s = static_cast<lzma_stream*>(stream_);
    total_in_ = s->total_in;
    total_out_ = s->total_out;
    src_begin = const_cast<const char*>(reinterpret_cast<const char*>(s->next_in));
    dest_begin = reinterpret_cast<char*>(s->next_out);
}

int lzma_base::deflate(lzma_action action)
{
    return lzma_code(static_cast<lzma_stream*>(stream_), action);
}

int lzma_base::inflate(lzma_action action)
{ 
    return lzma_code(static_cast<lzma_stream*>(stream_), action);
}

void lzma_base::reset(bool compress, bool realloc)
{
    lzma_stream* s = static_cast<lzma_stream*>(stream_);
    if (realloc)
        lzma_error::check(lzma_code(s, lzma::full_flush));
    else
        lzma_end(s);
}

void lzma_base::do_init
    ( const lzma_params& p, bool compress, 
      #if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
          lzma::alloc_func alloc, lzma::free_func free, 
      #endif
      void* derived )
{
    lzma_stream* s = static_cast<lzma_stream*>(stream_);

    memset(s, 0, sizeof(*s));

    lzma_error::check(
        compress ?
            lzma_easy_encoder(s, p.level, LZMA_CHECK_CRC32) :
            lzma_stream_decoder(s, 100 * 1024 * 1024, 0 )
    );
}

} // End namespace detail.

//----------------------------------------------------------------------------//

} } // End namespaces iostreams, boost.

#endif

