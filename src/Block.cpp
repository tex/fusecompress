#include <boost/io/ios_state.hpp>

#include "Block.hpp"

std::ostream &operator<< (std::ostream& os, const Block& rBl)
{
	boost::io::ios_flags_saver ifs(os);

	os << std::hex << "offset: 0x" << rBl.offset
	               << ", length: 0x" << rBl.length
	               << ", olength: 0x" << rBl.olength
	               << ", level: 0x" << rBl.level
	               << ", coffset: 0x" << rBl.coffset
	               << ", clength: 0x" << rBl.clength;
	return os;
}

