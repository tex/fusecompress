#ifndef FILEHEADER_HPP
#define FILEHEADER_HPP

#include <sys/types.h>
#include <cassert>

#include <boost/serialization/access.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/version.hpp>

#include <iostream>

#include "CompressionType.hpp"

using namespace std;

class FileHeader
{
	friend class boost::serialization::access;

	// TODO: xattr

	template<class Archive>
	void load(Archive &ar, const unsigned /*version*/)
	{
		ar >> id_0 >> id_1 >> id_2;

		if (isValid())
		{
			ar >> size >> index >> type;
		}
	}
	template<class Archive>
	void save(Archive &ar, const unsigned /*version*/) const
	{
		assert(isValid());

		ar << id_0 << id_1 << id_2;
		ar << size << index << type;
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	static const char *m_pExtAttrName;
public:
	FileHeader();
	~FileHeader() {};

	/*
	 * Identify header.
	 *
	 * Return:
	 * 	- True if header is correct (file is FuseCompress format).
	 * 	- False if header is not correct.
	 */
	inline bool isValid() const
	{
		if ((id_0 != '\037') || (id_1 != '\135') || (id_2 != '\211'))
			return false;
		return true;
	}

	inline void acquire(const FileHeader& src)
	{
		id_0 = src.id_0;
		id_1 = src.id_1;
		id_2 = src.id_2;
		size = src.size;
		index = src.index;
		type.acquire(src.type);
	}

	char		id_0;
	char		id_1;
	char		id_2;	// FuseCompress identification
	off_t		size;	// Length of the uncompressed file
	off_t		index;	// Position of the index in the compressed file
				// (0 - index is not present in the file)
	CompressionType	type;	// Compression type of the index

	// portable_binary_oarchive stores a number of bytes used to
	// store a number before number itself. So we have to add a byte
	// for every int / uint / long /ulong number...

	static const int MaxSize = 3 + 8 + 1 + 8 + 1 + CompressionType::MaxSize;
};

// Don't need versioning info for the FileHeader.

BOOST_CLASS_IMPLEMENTATION(FileHeader, boost::serialization::object_serializable)

// Don't track instances of this class. Users creates
// them on the stack.

BOOST_CLASS_TRACKING(FileHeader, boost::serialization::track_never) 

#endif

