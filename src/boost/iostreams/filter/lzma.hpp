// (C) Copyright Milan Svoboda 2008.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Note: custom allocators are not supported on VC6, since that compiler
// had trouble finding the function lzma_base::do_init.

#ifndef BOOST_IOSTREAMS_LZMA_HPP_INCLUDED
#define BOOST_IOSTREAMS_LZMA_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif              

#include <lzma.h>

#include <cassert>                            
#include <iosfwd>            // streamsize.                 
#include <memory>            // allocator, bad_alloc.
#include <new>          
#include <boost/config.hpp>  // MSVC, STATIC_CONSTANT, DEDUCED_TYPENAME, DINKUM.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/constants.hpp>   // buffer size.
#include <boost/iostreams/detail/config/auto_link.hpp>
#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include <boost/iostreams/detail/ios.hpp>  // failure, streamsize.
#include <boost/iostreams/filter/symmetric.hpp>                
#include <boost/iostreams/pipeline.hpp>                
#include <boost/type_traits/is_same.hpp>

// Must come last.
#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable:4251 4231 4660)         // Dependencies not exported.
#endif
#include <boost/config/abi_prefix.hpp>           

namespace boost { namespace iostreams {

namespace lzma {

typedef void* (*alloc_func)(void*, size_t, size_t);
typedef void (*free_func)(void*, void*);

                    // Compression levels

BOOST_IOSTREAMS_DECL extern const lzma_easy_level no_compression;
BOOST_IOSTREAMS_DECL extern const lzma_easy_level best_speed;
BOOST_IOSTREAMS_DECL extern const lzma_easy_level best_compression;
BOOST_IOSTREAMS_DECL extern const lzma_easy_level default_compression;

                    // Status codes

BOOST_IOSTREAMS_DECL extern const int okay;
BOOST_IOSTREAMS_DECL extern const int stream_end;
BOOST_IOSTREAMS_DECL extern const int unsupported_check;
BOOST_IOSTREAMS_DECL extern const int mem_error;
BOOST_IOSTREAMS_DECL extern const int header_error;
BOOST_IOSTREAMS_DECL extern const int data_error;
BOOST_IOSTREAMS_DECL extern const int buf_error;
BOOST_IOSTREAMS_DECL extern const int prog_error;

                    // Flush codes

BOOST_IOSTREAMS_DECL extern const lzma_action finish;
BOOST_IOSTREAMS_DECL extern const lzma_action full_flush;
BOOST_IOSTREAMS_DECL extern const lzma_action sync_flush;
BOOST_IOSTREAMS_DECL extern const lzma_action run;

                    // Code for current OS

                    // Null pointer constant.

const int null                               = 0;

                    // Default values

} // End namespace lzma. 

//
// Class name: lzma_params.
// Description: Encapsulates the parameters passed to lzmadec_init
//      to customize compression and decompression.
//
struct lzma_params {

    // Non-explicit constructor.
    lzma_params( lzma_easy_level level = lzma::default_compression )
        : level(level)
        { }
    lzma_easy_level level;
};

//
// Class name: lzma_error.
// Description: Subclass of std::ios::failure thrown to indicate
//     lzma errors other than out-of-memory conditions.
//
class BOOST_IOSTREAMS_DECL lzma_error : public BOOST_IOSTREAMS_FAILURE {
public:
    explicit lzma_error(int error);
    int error() const { return error_; }
    static void check(int error);
private:
    int error_;
};

namespace detail {

template<typename Alloc>
struct lzma_allocator_traits {
#ifndef BOOST_NO_STD_ALLOCATOR
    typedef typename Alloc::template rebind<char>::other type;
#else
    typedef std::allocator<char> type;
#endif
};

template< typename Alloc,
          typename Base = // VC6 workaround (C2516)
              BOOST_DEDUCED_TYPENAME lzma_allocator_traits<Alloc>::type >
struct lzma_allocator : private Base {
private:
    typedef typename Base::size_type size_type;
public:
    BOOST_STATIC_CONSTANT(bool, custom = 
        (!is_same<std::allocator<char>, Base>::value));
    typedef typename lzma_allocator_traits<Alloc>::type allocator_type;
    static void* allocate(void* self, size_t items, size_t size);
    static void deallocate(void* self, void* address);
};

class BOOST_IOSTREAMS_DECL lzma_base { 
public:
    typedef char char_type;
protected:
    lzma_base();
    ~lzma_base();
    void* stream() { return stream_; }
    template<typename Alloc> 
    void init( const lzma_params& p, 
               bool compress,
               lzma_allocator<Alloc>& zalloc )
        {
            bool custom = lzma_allocator<Alloc>::custom;
            do_init( p, compress,
                     #if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
                         custom ? lzma_allocator<Alloc>::allocate : 0,
                         custom ? lzma_allocator<Alloc>::deallocate : 0,
                     #endif
                     &zalloc );
        }
    void before( const char*& src_begin, const char* src_end,
                 char*& dest_begin, char* dest_end );
    void after( const char*& src_begin, char*& dest_begin, 
                bool compress );
    int deflate(lzma_action action);
    int inflate(lzma_action action);
    void reset(bool compress, bool realloc);
public:
    unsigned long crc() const { return crc_; }
    int total_in() const { return total_in_; }
    int total_out() const { return total_out_; }
private:
    void do_init( const lzma_params& p, bool compress, 
                  #if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
                      lzma::alloc_func, 
                      lzma::free_func, 
                  #endif
                  void* derived );
    void*         stream_;         // Actual type: lzmadec_stream*.
    bool          calculate_crc_;
    unsigned long crc_; 
    int           total_in_;
    int           total_out_;
};

//
// Template name: lzma_compressor_impl
// Description: Model of C-Style Filter implementing compression by
//      delegating to the lzma function deflate.
//
template<typename Alloc = std::allocator<char> >
class lzma_compressor_impl : public lzma_base, public lzma_allocator<Alloc> { 
public: 
    lzma_compressor_impl(const lzma_params& = lzma::default_compression);
    ~lzma_compressor_impl();
    bool filter( const char*& src_begin, const char* src_end,
                 char*& dest_begin, char* dest_end, bool flush );
    void close();
};

//
// Template name: lzma_compressor_impl
// Description: Model of C-Style Filte implementing decompression by
//      delegating to the lzma function inflate.
//
template<typename Alloc = std::allocator<char> >
class lzma_decompressor_impl : public lzma_base, public lzma_allocator<Alloc> {
public:
    lzma_decompressor_impl(const lzma_params&);
    lzma_decompressor_impl();
    ~lzma_decompressor_impl();
    bool filter( const char*& begin_in, const char* end_in,
                 char*& begin_out, char* end_out, bool flush );
    void close();
};

} // End namespace detail.

//
// Template name: lzma_compressor
// Description: Model of InputFilter and OutputFilter implementing
//      compression using lzma.
//
template<typename Alloc = std::allocator<char> >
struct basic_lzma_compressor 
    : symmetric_filter<detail::lzma_compressor_impl<Alloc>, Alloc> 
{
private:
    typedef detail::lzma_compressor_impl<Alloc> impl_type;
    typedef symmetric_filter<impl_type, Alloc>  base_type;
public:
    typedef typename base_type::char_type               char_type;
    typedef typename base_type::category                category;
    basic_lzma_compressor( const lzma_params& = lzma::default_compression, 
                           int buffer_size = default_device_buffer_size );
    unsigned long crc() { return this->filter().crc(); }
    int total_in() {  return this->filter().total_in(); }
};
BOOST_IOSTREAMS_PIPABLE(basic_lzma_compressor, 1)

typedef basic_lzma_compressor<> lzma_compressor;

//
// Template name: lzma_decompressor
// Description: Model of InputFilter and OutputFilter implementing
//      decompression using lzma.
//
template<typename Alloc = std::allocator<char> >
struct basic_lzma_decompressor 
    : symmetric_filter<detail::lzma_decompressor_impl<Alloc>, Alloc> 
{
private:
    typedef detail::lzma_decompressor_impl<Alloc> impl_type;
    typedef symmetric_filter<impl_type, Alloc>    base_type;
public:
    typedef typename base_type::char_type               char_type;
    typedef typename base_type::category                category;
    basic_lzma_decompressor( int buffer_size = default_device_buffer_size );
    basic_lzma_decompressor( const lzma_params& p,
                             int buffer_size = default_device_buffer_size );
    unsigned long crc() { return this->filter().crc(); }
    int total_out() {  return this->filter().total_out(); }
};
BOOST_IOSTREAMS_PIPABLE(basic_lzma_decompressor, 1)

typedef basic_lzma_decompressor<> lzma_decompressor;

//----------------------------------------------------------------------------//

//------------------Implementation of lzma_allocator--------------------------//

namespace detail {

template<typename Alloc, typename Base>
void* lzma_allocator<Alloc, Base>::allocate
    (void* self, size_t items, size_t size)
{ 
    size_type len = items * size;
    char* ptr = 
        static_cast<allocator_type*>(self)->allocate
            (len + sizeof(size_type)
            #if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
                , (char*)0
            #endif
            );
    *reinterpret_cast<size_type*>(ptr) = len;
    return ptr + sizeof(size_type);
}

template<typename Alloc, typename Base>
void lzma_allocator<Alloc, Base>::deallocate(void* self, void* address)
{ 
    char* ptr = reinterpret_cast<char*>(address) - sizeof(size_type);
    size_type len = *reinterpret_cast<size_type*>(ptr) + sizeof(size_type);
    static_cast<allocator_type*>(self)->deallocate(ptr, len); 
}

//------------------Implementation of lzma_compressor_impl--------------------//

template<typename Alloc>
lzma_compressor_impl<Alloc>::lzma_compressor_impl(const lzma_params& p)
{ init(p, true, static_cast<lzma_allocator<Alloc>&>(*this)); }

template<typename Alloc>
lzma_compressor_impl<Alloc>::~lzma_compressor_impl()
{ /* reset(true, false); */ }

template<typename Alloc>
bool lzma_compressor_impl<Alloc>::filter
    ( const char*& src_begin, const char* src_end,
      char*& dest_begin, char* dest_end, bool flush )
{
    before(src_begin, src_end, dest_begin, dest_end);
    int result = deflate(flush ? lzma::finish : lzma::run);
    after(src_begin, dest_begin, true);
    lzma_error::check(result);
    return result != lzma::stream_end; 
}

template<typename Alloc>
void lzma_compressor_impl<Alloc>::close() { reset(true, false); }

//------------------Implementation of lzma_decompressor_impl------------------//

template<typename Alloc>
lzma_decompressor_impl<Alloc>::lzma_decompressor_impl(const lzma_params& p)
{ init(p, false, static_cast<lzma_allocator<Alloc>&>(*this)); }

template<typename Alloc>
lzma_decompressor_impl<Alloc>::~lzma_decompressor_impl()
{ /* reset(false, false); */ }

template<typename Alloc>
lzma_decompressor_impl<Alloc>::lzma_decompressor_impl()
{ 
    lzma_params p;
    init(p, false, static_cast<lzma_allocator<Alloc>&>(*this)); 
}

template<typename Alloc>
bool lzma_decompressor_impl<Alloc>::filter
    ( const char*& src_begin, const char* src_end,
      char*& dest_begin, char* dest_end, bool /* flush */ )
{
    before(src_begin, src_end, dest_begin, dest_end);
    int result = inflate(lzma::run);
    after(src_begin, dest_begin, false);
    lzma_error::check(result);
    return result != lzma::stream_end;
}

template<typename Alloc>
void lzma_decompressor_impl<Alloc>::close() { reset(false, false); }

} // End namespace detail.

//------------------Implementation of lzma_compressor-----------------------//

template<typename Alloc>
basic_lzma_compressor<Alloc>::basic_lzma_compressor
    (const lzma_params& p, int buffer_size) 
    : base_type(buffer_size, p) { }

//------------------Implementation of lzma_decompressor-----------------------//

template<typename Alloc>
basic_lzma_decompressor<Alloc>::basic_lzma_decompressor
    (int buffer_size) 
    : base_type(buffer_size) { }

template<typename Alloc>
basic_lzma_decompressor<Alloc>::basic_lzma_decompressor
    (const lzma_params& p, int buffer_size) 
    : base_type(buffer_size, p) { }

//----------------------------------------------------------------------------//

} } // End namespaces iostreams, boost.

#include <boost/config/abi_suffix.hpp> // Pops abi_suffix.hpp pragmas.
#ifdef BOOST_MSVC
# pragma warning(pop)
#endif

#endif // #ifndef BOOST_IOSTREAMS_ZLIB_HPP_INCLUDED
