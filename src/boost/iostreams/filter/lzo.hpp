// (C) Copyright Milan Svoboda 2009.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef LZO_FILTER_HPP
#define LZO_FILTER_HPP

#include <lzo/lzo1x.h>

#define HEAP_ALLOC(var,size) \
	lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

#include <boost/iostreams/filter/aggregate.hpp>

namespace boost { namespace iostreams {

template< typename Ch, typename Alloc = std::allocator<Ch> >
class basic_lzo_compressor : public boost::iostreams::aggregate_filter<Ch, Alloc>
{
   private:

      typedef boost::iostreams::aggregate_filter<Ch, Alloc> base_type;

   public:

      typedef typename base_type::char_type char_type;
      typedef typename base_type::category category;
      typedef std::basic_string<Ch> string_type;

   public:

      basic_lzo_compressor()
      { }

   private:

      typedef typename base_type::vector_type vector_type;

      void do_filter(const vector_type& src, vector_type& dest)
      {
         lzo_uint len = src.size() + src.size() / 16 + 64 + 3;

         HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);
         vector_type tmp(len);

         lzo1x_1_compress((lzo_bytep) &src[0], src.size(),
                          (lzo_bytep) &tmp[0], &len, wrkmem);

         dest.reserve(len);
         for (unsigned int i = 0; i < len; ++i)
            dest.push_back(tmp[i]);
      }

   private:

};

BOOST_IOSTREAMS_PIPABLE(basic_lzo_compressor, 1)

typedef basic_lzo_compressor<char> lzo_compressor;
typedef basic_lzo_compressor<wchar_t> wlzo_compressor;

//

template< typename Ch, typename Alloc = std::allocator<Ch> >
class basic_lzo_decompressor : public boost::iostreams::aggregate_filter<Ch, Alloc>
{
   private:

      typedef boost::iostreams::aggregate_filter<Ch, Alloc> base_type;

   public:

      typedef typename base_type::char_type char_type;
      typedef typename base_type::category category;
      typedef std::basic_string<Ch> string_type;

   public:

      basic_lzo_decompressor()
      { }

   private:

      typedef typename base_type::vector_type vector_type;

      void do_filter(const vector_type& src, vector_type& dest)
      {
         vector_type tmp;
         lzo_uint len;
         int compressionFactor = 10;

         while (true)
         {
             len = src.size() * compressionFactor;
             tmp.assign(len, 0);

             if (lzo1x_decompress_safe((lzo_bytep) &src[0], src.size(),
                                       (lzo_bytep) &tmp[0], &len, NULL) == LZO_E_OUTPUT_OVERRUN)
             {
                compressionFactor *= 2;
                continue;
             }
             break;
         }

         dest.assign(len, 0);
         for (unsigned int i = 0; i < len; ++i)
            dest[i] = tmp[i];
      }

   private:

};

BOOST_IOSTREAMS_PIPABLE(basic_lzo_decompressor, 1)

typedef basic_lzo_decompressor<char> lzo_decompressor;
typedef basic_lzo_decompressor<wchar_t> wlzo_decompressor;


} }

#endif

