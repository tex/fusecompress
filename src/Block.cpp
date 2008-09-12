#include <stdint.h>
#include <cassert>
#include <cstring>

#include "ByteOrder.hpp"
#include "Block.hpp"

using namespace std;

// Store in little-endian byteorder on disk.
// 
struct packed_block
{
	uint64_t	offset;
	uint32_t	length;
	uint32_t	olength;
	uint64_t	coffset;
	uint32_t	clength;
	uint32_t	level;

} __attribute__((packed));

Block::Block(off_t offset, size_t length, off_t coffset, size_t clength) :
	offset (offset),
	coffset (coffset),
	length (length),
	olength (length),
	clength (clength),
	level (0)
{
}

Block::Block(off_t offset, size_t length) :
	offset (offset),
	coffset (0),
	length (length),
	olength (length),
	clength (0),
	level (0)
{
}

Block::Block(off_t offset, size_t length, unsigned int level) :
	offset (offset),
	coffset (0),
	length (length),
	olength (length),
	clength (0),
	level (level)
{
}

size_t Block::size()
{
	return sizeof(struct packed_block);
}

void Block::store(struct packed_block &pb) const
{
	pb.offset = cpu_to_le64(offset);
	pb.length = cpu_to_le32(length);
	pb.olength = cpu_to_le32(olength);
	pb.coffset = cpu_to_le64(coffset);
	pb.clength = cpu_to_le32(clength);
	pb.level = cpu_to_le32(level);
}

void Block::restore(struct packed_block &pb)
{
	offset = le64_to_cpu(pb.offset);
	length = le32_to_cpu(pb.length);
	olength = le32_to_cpu(pb.olength);
	coffset = le64_to_cpu(pb.coffset);
	clength = le32_to_cpu(pb.clength);
	level = le32_to_cpu(pb.level);
}

off_t Block::store(int fd, off_t to) const
{
	ssize_t r;
	struct packed_block pb;

	store(pb);
	
	r = pwrite(fd, &pb, sizeof pb, to);
	if (r != sizeof pb)
		return -1;

	return to + r;
}

off_t Block::restore(int fd, off_t from)
{
	ssize_t r;
	struct packed_block pb;

	r = pread(fd, &pb, sizeof pb, from);
	if (r != sizeof pb)
		return -1;

	restore(pb);

	return from + r;
}

size_t Block::store(char *buf, size_t len) const
{
	struct packed_block pb;

	assert (len >= sizeof pb);

	store(pb);

	memcpy(buf, &pb, sizeof pb);
	
	return sizeof pb;
}

size_t Block::restore(const char *buf, size_t len)
{
	struct packed_block pb;

	assert (len >= sizeof pb);

	memcpy(&pb, buf, sizeof pb);

	restore(pb);

	return sizeof pb;
}

void Block::Print(ostream &os) const
{
	os << "offset: 0x" << hex << offset;
	os << ", length: 0x" << hex << length;
	os << ", olength: 0x" << hex << olength;
	os << ", level: 0x" << hex << level;
	os << ", coffset: 0x" << hex << coffset;
	os << ", clength: 0x" << hex << clength;
}

ostream &operator<< (ostream &os, const Block &rBl)
{
	rBl.Print(os);
	return os;
}
