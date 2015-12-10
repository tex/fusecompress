/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    FuseCompress is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FuseCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

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

