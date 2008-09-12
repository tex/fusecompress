#ifndef BLOCKHPP
#define BLOCKHPP

#include <iostream>

using namespace std;

class Block
{
	void store(struct packed_block &pb) const;
	void restore(struct packed_block &pb);
public:
	Block() : level (0) { };
	Block(off_t offset, size_t length, off_t coffset, size_t clength);
	Block(off_t offset, size_t length);
	Block(off_t offset, size_t length, unsigned int level);
	~Block() { };

	off_t offset, coffset;
	size_t length, olength, clength;

	unsigned int level;

	/**
	 * Return size of the Block stored on disk in bytes.
	 */
	static size_t size();
	
	off_t store(int fd, off_t to) const;
	off_t restore(int fd, off_t from);

	size_t store(char *buf, size_t len) const;
	size_t restore(const char *buf, size_t len);

	/*
	 * Only for debugging purposes.
	 */
	void Print(ostream &os) const;
};

ostream &operator<< (ostream &os, const Block &rBl);

#endif

