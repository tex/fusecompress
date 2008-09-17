#ifndef XOR_FILTER_HPP
#define XOR_FILTER_HPP

#include <boost/iostreams/filter/aggregate.hpp>

namespace boost { namespace iostreams {

template< typename Ch, typename Alloc = std::allocator<Ch> >
class basic_xor_filter : public boost::iostreams::aggregate_filter<Ch, Alloc>
{
   private:

      typedef boost::iostreams::aggregate_filter<Ch, Alloc> base_type;

   public:

      typedef typename base_type::char_type char_type;
      typedef typename base_type::category category;
      typedef std::basic_string<Ch> string_type;

   public:

      basic_xor_filter(Ch key)
         : key_(key)
      { }

   private:

      typedef typename base_type::vector_type vector_type;

      void do_filter(const vector_type& src, vector_type& dest)
      {
         for (size_t i = 0; i < src.size(); ++i) {
            dest.push_back(src[i] ^ key_);
         }
      }

   private:

      Ch key_;
};

BOOST_IOSTREAMS_PIPABLE(basic_xor_filter, 1)

typedef basic_xor_filter<char> xor_filter;
typedef basic_xor_filter<wchar_t> wxor_filter;

} }

#endif

