#ifndef BOOST_IOSTREAMS_BYTESCOUNTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_BYTESCOUNTER_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/pipeline.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // VC7.1 C4244.

namespace boost { namespace iostreams {

//
// Template name: basic_counter.
// Template paramters:
//      Ch - The character type.
// Description: Filter which counts .
//
template<typename Ch>
class basic_bytescounter  {
public:
    typedef Ch char_type;
    struct category
        : dual_use,
          filter_tag,
          multichar_tag
        { };
    explicit basic_bytescounter(int bytes = 0)
        : bytes_(bytes)
        { }
    int bytes() const { return bytes_; }

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        std::streamsize result = iostreams::read(src, s, n);
        if (result == -1)
            return -1;
        bytes_ += result;
        return result;
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        std::streamsize result = iostreams::write(snk, s, n);
        bytes_ += result;
        return result;
    }
private:
    int bytes_;
};
BOOST_IOSTREAMS_PIPABLE(basic_bytescounter, 1)


typedef basic_bytescounter<char>     bytescounter;

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_BYTESCOUNTER_HPP_INCLUDED
