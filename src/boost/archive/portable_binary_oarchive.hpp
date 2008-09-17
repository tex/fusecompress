/*****************************************************************************/
/**
 * \file portable_binary_oarchive.hpp
 * \brief Provides an archive to create portable binary files.
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

#include <ostream>

#include <boost/archive/binary_oarchive.hpp>
#if BOOST_VERSION < 103500
#include <boost/archive/detail/polymorphic_oarchive_impl.hpp>
#else
#include <boost/archive/detail/polymorphic_oarchive_dispatch.hpp>
#endif

#include <boost/integer/endian.hpp>
#include <boost/math/fpclassify.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_floating_point.hpp>

/**
 * Portable binary output archive using little endian format.
 *
 * This archive addresses integer size, endianness and floating point types so
 * that data can be transferred across different systems. The archive consists
 * primarily of three different save implementations for integral types,
 * floating point types and string types. Those functions are templates and use
 * enable_if to be correctly selected for overloading.
 *
 * \note The class is based on the portable binary example by Robert Ramey and
 *       uses Beman Dawes endian library plus fp_utilities by Johan Rade.
 */
class portable_binary_oarchive :
    // don't derive from binary_oarchive !!!
    public boost::archive::binary_oarchive_impl<
		portable_binary_oarchive
#if BOOST_VERSION >= 103400
		, std::ostream::char_type
		, std::ostream::traits_type
#endif
	>
{
	// base class typedef
    typedef boost::archive::binary_oarchive_impl<
        portable_binary_oarchive
#if BOOST_VERSION >= 103400
        , std::ostream::char_type 
        , std::ostream::traits_type
#endif
    > archive_base_t;

	// binary primitive typedef
    typedef boost::archive::basic_binary_oprimitive<
        portable_binary_oarchive 
#if BOOST_VERSION < 103400
		, std::ostream
#else
        , std::ostream::char_type
        , std::ostream::traits_type
#endif
    > primitive_base_t;

	// This typedef
    typedef portable_binary_oarchive derived_t;

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
	friend archive_base_t; // needed for static polymorphism
	friend primitive_base_t; // since we override save below
#if BOOST_VERSION < 103500
	friend class boost::archive::detail::polymorphic_oarchive_impl<derived_t>;
#else
	friend class boost::archive::detail::polymorphic_oarchive_dispatch<derived_t>;
#endif
	friend class boost::archive::basic_binary_oarchive<derived_t>;
	friend class boost::archive::save_access;
#endif

	// workaround for gcc: use a dummy struct
	// as additional argument type for overloading
	template<int> struct dummy { dummy(int) {}};

    //! default fall through for non-arithmetic types (ie. strings)
    template<class T>
    BOOST_DEDUCED_TYPENAME boost::disable_if<boost::is_arithmetic<T> >::type 
	save(const T & t, dummy<1> = 0)
	{
        archive_base_t::save(t);
    }

	//! special case saving bool type, 
	//! preserving compatibility to integer types
	void save(const bool& b) 
	{ 
		char c = b; // implicit conversion
		archive_base_t::save(c); // size
		if (b) archive_base_t::save<char>('T'); // value
	}

	/**
     * save integer types
	 *
	 * First we save the size information ie. the number of bytes that hold the
	 * actual data. We subsequently transform the data using store_little_endian
	 * and store non-zero bytes to the stream.
     */
	template <typename T>
	BOOST_DEDUCED_TYPENAME boost::enable_if<boost::is_integral<T> >::type
	save(const T & t, dummy<2> = 0)
	{
        signed char size = 0;
		if (T temp = t)
		{
			// examine the number of bytes
			// needed to represent the number
			do { temp >>= 8; ++size; } 
			while (temp != -1 && temp != 0);

			// if the value is negative let the size be negative
			bool negative = boost::is_signed<T>::value && t < 0;
			archive_base_t::save(negative ? (signed char) -size : size);

			// we choose to use little endian because this way we just
			// save the first size bytes to the stream and skip the rest
			boost::detail::store_little_endian<T, sizeof(T)>(&temp, t);
			save_binary(&temp, size);
		}
		// optimization: store zero as size 0
		else archive_base_t::save(size);
	}

	/**
     * save floating point types
	 * 
	 * We simply rely on fp_traits to extract the bit pattern into an (unsigned)
	 * integral type and store that into the stream.
	 *
	 * \todo treat nan values using fp_classify
     */
	template <typename T>
	BOOST_DEDUCED_TYPENAME boost::enable_if<boost::is_floating_point<T> >::type
	save(const T & t, dummy<3> = 0)
	{
		using namespace boost::math::detail;
		BOOST_DEDUCED_TYPENAME fp_traits<T>::type::bits bits;
		fp_traits<T>::type::get_bits(t, bits);
		save(bits);
	}

public:
	//! always construct on a stream using std::ios::binary mode!
    portable_binary_oarchive(std::ostream& os, unsigned flags = 0)
		: archive_base_t(os, flags | boost::archive::no_header) {}
};

#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/archive_pointer_oserializer.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>

namespace boost {
namespace archive {

// explicitly instantiate for this type of binary stream
template class basic_binary_oarchive<portable_binary_oarchive> ;

template class basic_binary_oprimitive<
	portable_binary_oarchive
#if BOOST_VERSION < 103400
	, std::ostream
#else
	, std::ostream::char_type
	, std::ostream::traits_type
#endif
> ;
template class binary_oarchive_impl<
	portable_binary_oarchive
#if BOOST_VERSION >= 103400
	, std::ostream::char_type
	, std::ostream::traits_type
#endif
> ;
template class detail::archive_pointer_oserializer<portable_binary_oarchive> ;

} // namespace archive
} // namespace boost

#define BOOST_ARCHIVE_CUSTOM_OARCHIVE_TYPES portable_binary_oarchive

// polymorphic portable binary oarchive typedef
#if BOOST_VERSION < 103500
typedef boost::archive::detail::polymorphic_oarchive_impl<
	portable_binary_oarchive> polymorphic_portable_binary_oarchive;
#else
typedef boost::archive::detail::polymorphic_oarchive_dispatch<portable_binary_oarchive> 
	polymorphic_portable_binary_oarchive;
#endif
