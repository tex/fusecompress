/*****************************************************************************/
/**
 * \file portable_binary_iarchive.hpp
 * \brief Provides an archive to read from portable binary files.
 * \author christian.pfligersdorffer@eos.info
 * \version 2.0
 *
 * This archive (pair) brings the advantanges of binary streams to the cross
 * platform boost::serialization user. While being almost as fast as the native
 * binary archive it allows its files to be exchanged between cpu architectures
 * using different byte order (endianness). Speaking of speed: in serializing
 * numbers the (portable) binary approach is approximately ten times faster than
 * the ascii implementation that is inherently portable.
 *
 * Based on the portable archive example by Robert Ramey this implementation
 * uses Beman Dawes endian library and fp_utilities from Johan Rade, of which
 * the latter has become part of the boost libraries as of version 1.35. But,
 * since this extension has only been tested with boost versions 1.33 and 1.34,
 * you need to add both (header only) modules to your boost directory manually.
 *
 * \note Correct behaviour has been confirmed using PowerPC-32 and x86-32
 *       platforms featuring big endian and little endian byte order. It might
 *       or might not instantly work for your specific setup. If you encounter
 *       problems or have suggestions please contact the author.
 *
 * \note Version 1.x contained a serious bug that effectively transformed most
 *       of negative integral values into positive values! For example the two
 *       numbers -12 and 234 were stored in the same 8-bit pattern and later
 *       always restored to 234. This was fixed in this version in a way that
 *       does not change the interpretation of existing archives that did work
 *       because there were no negative numbers. The other way round archives
 *       created by version 2.0 and containing negative numbers will raise an
 *       integer type size exception when reading it with version 1.x. Thanks
 *       to Markus Frohnmaier for testing the archives and finding the bug.
 *
 * \copyright The boost software license applies.
 */
/*****************************************************************************/

#pragma once

#include <istream>

#include <boost/version.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#if BOOST_VERSION < 103500
#include <boost/archive/detail/polymorphic_iarchive_impl.hpp>
#elif BOOST_VERSION < 103600
#include <boost/archive/detail/polymorphic_iarchive_dispatch.hpp>
#else
#include <boost/archive/detail/polymorphic_iarchive_route.hpp>
#endif

#include <boost/integer/endian.hpp>
#include <boost/math/fpclassify.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_floating_point.hpp>

/**
 * Exception being thrown when an incompatible integer size is encountered.
 *
 * Note that it will also be thrown if you mixed up your stream position and
 * accidentially interpret some value for size data (in this case invalid_size
 * will be more than one digit most of the time).
 */
class portable_binary_archive_exception : public boost::archive::archive_exception
{
public:
	//! type size is not large enough for deserialized number
    portable_binary_archive_exception(unsigned int invalid_size) 
		: boost::archive::archive_exception(other_exception) 
		, msg("requested integer size exceeds type size: ")
	{
		msg += boost::lexical_cast<std::string>(invalid_size);
	}

	//! negative number in unsigned type
	portable_binary_archive_exception()
		: boost::archive::archive_exception(other_exception)
		, msg("cannot read a negative number into an unsigned type")
	{
	}

	//! override the base class function with own message
    const char* what() const throw() { return msg.c_str(); }
	~portable_binary_archive_exception() throw() {}

private:
	std::string msg;
};

/**
 * Portable binary input archive using little endian format.
 *
 * This archive addresses integer size, endianness and floating point types so
 * that data can be transferred across different systems. There may still be
 * constraints as to what systems are compatible and the user will have to take
 * care that e.g. a very large int being saved on a 64 bit machine will result
 * in a portable_binary_archive_exception if loaded into an int on a 32 bit
 * system. A possible workaround to this would be to use fixed types like 
 * boost::uint64_t in your serialization structures.
 *
 * \note The class is based on the portable binary example by Robert Ramey and
 *	     uses Beman Dawes endian library plus fp_utilities by Johan Rade.
 */
class portable_binary_iarchive : 
    // don't derive from binary_iarchive !!!
	public boost::archive::binary_iarchive_impl<
		portable_binary_iarchive
#if BOOST_VERSION >= 103400
		, std::istream::char_type 
        , std::istream::traits_type
#endif
	>
{
	// base class typedef
    typedef boost::archive::binary_iarchive_impl<
        portable_binary_iarchive
#if BOOST_VERSION >= 103400
        , std::istream::char_type 
        , std::istream::traits_type
#endif
    > archive_base_t;

	// binary primitive typedef
    typedef boost::archive::basic_binary_iprimitive<
        portable_binary_iarchive 
#if BOOST_VERSION < 103400
		, std::istream
#else
        , std::ostream::char_type
        , std::ostream::traits_type
#endif
    > primitive_base_t;

	// This typedef
    typedef portable_binary_iarchive derived_t;
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
	friend archive_base_t; // needed for static polymorphism
	friend primitive_base_t; // since with override load below
#if BOOST_VERSION < 103500
	friend class boost::archive::detail::polymorphic_iarchive_impl<derived_t>;
#elif BOOST_VERSION < 103600
	friend class boost::archive::detail::polymorphic_iarchive_dispatch<derived_t>;
#else
	friend class boost::archive::detail::polymorphic_iarchive_route<derived_t>;
#endif
	friend class boost::archive::basic_binary_iarchive<derived_t>;
	friend class boost::archive::load_access;
#endif

	// workaround for gcc: use a dummy struct
	// as additional argument type for overloading
	template <int> struct dummy { dummy(int) {}};

    //! default fall through for non-arithmetic types (ie. strings)
    template <class T>
    BOOST_DEDUCED_TYPENAME boost::disable_if<boost::is_arithmetic<T> >::type 
	load(T & t, dummy<1> = 0)
	{
        archive_base_t::load(t);
    }

	//! special case loading bool type, preserving compatibility
	//! to integer types - this is somewhat redundant but simply
	//! treating bool as integer type generates some warnings
	void load(bool& b) 
	{ 
		char c; 
		archive_base_t::load(c); // size
		if (c == 1) archive_base_t::load(c); // value
		else if (c) throw portable_binary_archive_exception(c);
		b = (c != 0); 
	}

	/**
	 * load integer types
	 *
	 * First we load the size information ie. the number of bytes that 
	 * hold the actual data. Then we retrieve the data and transform it
	 * to the original value by using load_little_endian.
	 */
	template <typename T>
	BOOST_DEDUCED_TYPENAME boost::enable_if<boost::is_integral<T> >::type
	load(T & t, dummy<2> = 0)
	{
		// get the number of bytes following
		signed char size = 0;
		archive_base_t::load(size);

		if (size)
		{
			// check for possible deserialization errors
			if (size < 0 && !boost::is_signed<T>::value)
				throw portable_binary_archive_exception();
			if ((unsigned char) abs(size) > sizeof(T)) 
				throw portable_binary_archive_exception(size);

			// temporary holder of the value
			T temp = size < 0 ? -1 : 0;
			load_binary(&temp, abs(size));

			// load the value from little endian - is is then converted
			// to the target type T and fits it because size <= sizeof(T)
			t = boost::detail::load_little_endian<T, sizeof(T)>(&temp);
		}
		else t = 0; // zero optimization
	}

	/** 
	 * load floating point types
	 * 
	 * We simply rely on fp_traits to set the bit pattern from the
	 * (unsigned) integral type that was stored in the stream.
	 *
	 * \todo treat nan values using fp_classify
	 */
	template <typename T>
	BOOST_DEDUCED_TYPENAME boost::enable_if<boost::is_floating_point<T> >::type
	load(T & t, dummy<3> = 0)
	{
		using namespace boost::math::detail;
		BOOST_DEDUCED_TYPENAME fp_traits<T>::type::bits bits;
		load(bits);
		fp_traits<T>::type::set_bits(t, bits);
	}

public:
	//! always construct on a stream using std::ios::binary mode!
    portable_binary_iarchive(std::istream & is, unsigned flags = 0)
		: archive_base_t(is, flags | boost::archive::no_header) {}
};

#include <boost/archive/impl/basic_binary_iarchive.ipp>
#include <boost/archive/impl/archive_pointer_iserializer.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>

namespace boost {
namespace archive {

// explicitly instantiate for this type of binary stream
template class basic_binary_iarchive<portable_binary_iarchive> ;

template class basic_binary_iprimitive<
	portable_binary_iarchive
#if BOOST_VERSION < 103400
	, std::istream
#else
    , std::istream::char_type 
    , std::istream::traits_type
#endif
> ;

template class binary_iarchive_impl<
    portable_binary_iarchive
#if BOOST_VERSION >= 103400
    , std::istream::char_type 
    , std::istream::traits_type
#endif
>;

template class detail::archive_pointer_iserializer<portable_binary_iarchive> ;

} // namespace archive
} // namespace boost

#define BOOST_ARCHIVE_CUSTOM_IARCHIVE_TYPES portable_binary_iarchive

// polymorphic portable binary iarchive typdef
#if BOOST_VERSION < 103500
typedef boost::archive::detail::polymorphic_iarchive_impl<
	portable_binary_iarchive> polymorphic_portable_binary_iarchive;
#elif BOOST_VERSION < 103600
typedef boost::archive::detail::polymorphic_iarchive_dispatch<portable_binary_iarchive> 
	polymorphic_portable_binary_iarchive;
#else
typedef boost::archive::detail::polymorphic_iarchive_route<portable_binary_iarchive>
        polymorphic_portable_binary_iarchive;
#endif
