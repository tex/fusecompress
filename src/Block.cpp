#include <boost/io/ios_state.hpp>

#include "Block.hpp"

void Block::Print(std::ostream& os) const
{
	boost::io::ios_flags_saver ifs(os);

	os << std::hex << "offset: 0x" << offset
	               << ", length: 0x" << length
	               << ", olength: 0x" << olength
	               << ", level: 0x" << level
	               << ", coffset: 0x" << coffset;
}

std::ostream &operator<< (std::ostream& os, const Block& rBl)
{
	rBl.Print(os);
	return os;
}

