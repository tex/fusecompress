#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "LayerMap.hpp"

BOOST_AUTO_TEST_CASE(t1)
{
	LayerMap m;

	Block block;
	off_t length;
/*
	0    15  20    25        30
	      |---------|
	                |---------|
 */
	m.Put(new Block(15, 10));
	m.Put(new Block(25, 5));

	m.Get(0, block, length);

	BOOST_CHECK(block.offset == 15);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 0);
}

BOOST_AUTO_TEST_CASE(t2)
{
	LayerMap m;

	Block block;
	off_t length;
/*
	0    5   10   15  20   25
	     |---------|
	               |---------|
 */
	m.Put(new Block(5, 10));
	m.Put(new Block(15, 5));

	m.Get(0, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 0);
}

BOOST_AUTO_TEST_CASE(t3)
{
	LayerMap m;

	Block        block;
	off_t length;
	bool         r;

/*
	0    5    10   15
	|----------|
	     |----------|
 */

	m.Put(new Block(0, 10));
	m.Put(new Block(5, 10));

	m.Get(0, block, length);

	BOOST_CHECK(block.offset == 0);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 5);

	m.Get(4, block, length);

	BOOST_CHECK(block.offset == 0);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 1);

	m.Get(5, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 10);

	m.Get(6, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 9);

	m.Get(9, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 6);

	m.Get(10, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 5);

	m.Get(11, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 4);

	r = m.Get(15, block, length);

	BOOST_CHECK(r == false);
	BOOST_CHECK(block.offset == 5);	// Unmodified previous result
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 4);
}

BOOST_AUTO_TEST_CASE(t4)
{
	LayerMap m;

	Block        block;
	off_t length;
/*
	0    5   10   15  20   25
	|---------|
	     |---------|
                           |----|
 */
	m.Put(new Block(0, 10));
	m.Put(new Block(5, 10));
	m.Put(new Block(20, 5));

	m.Get(14, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 1);

	m.Get(15, block, length);

	BOOST_CHECK(block.offset == 20);
	BOOST_CHECK(block.length == 5);
	BOOST_CHECK(length == 0);
}

BOOST_AUTO_TEST_CASE(t5)
{
	LayerMap m;

	Block        block;
	off_t length;

/*
	0   5   10   15    20 24 25
	|--------|
	    |---------|
                            |---|
	         |-------------|
 */
	m.Put(new Block(0, 10));
	m.Put(new Block(5, 10));
	m.Put(new Block(20, 5));
	m.Put(new Block(10, 14));

	m.Get(15, block, length);

	BOOST_CHECK(block.offset == 10);
	BOOST_CHECK(block.length == 14);
	BOOST_CHECK(length == 9);

	m.Get(20, block, length);

	BOOST_CHECK(block.offset == 10);
	BOOST_CHECK(block.length == 14);
	BOOST_CHECK(length == 4);

	m.Get(24, block, length);

	BOOST_CHECK(block.offset == 20);
	BOOST_CHECK(block.length == 5);
	BOOST_CHECK(length == 1);
}

BOOST_AUTO_TEST_CASE(t6)
{
	LayerMap m;

	Block        block;
	off_t length;

/*
        0    5  10  15 17 19 25 30
	|--------|
	             |-----------|
	     |------------|
	               |------|

 */
	m.Put(new Block(0, 10));
	m.Put(new Block(15, 15));
	m.Put(new Block(5, 14));
	m.Put(new Block(17, 7));

	m.Get(3, block, length);

	BOOST_CHECK(block.offset == 0);
	BOOST_CHECK(block.length == 10);
	BOOST_CHECK(length == 2);

	m.Get(5, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 14);
	BOOST_CHECK(length == 12);

	m.Get(9, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 14);
	BOOST_CHECK(length == 8);

	m.Get(14, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 14);
	BOOST_CHECK(length == 3);

	m.Get(15, block, length);

	BOOST_CHECK(block.offset == 5);
	BOOST_CHECK(block.length == 14);
	BOOST_CHECK(length == 2);

	m.Get(17, block, length);

	BOOST_CHECK(block.offset == 17);
	BOOST_CHECK(block.length == 7);
	BOOST_CHECK(length == 7);

	m.Get(25, block, length);

	BOOST_CHECK(block.offset == 15);
	BOOST_CHECK(block.length == 15);
	BOOST_CHECK(length == 5);
}

BOOST_AUTO_TEST_CASE(t7)
{
	LayerMap m;

	Block        block;
	off_t length;
	bool         r;

	m.Put(new Block(0x2a50, 0x5b0, 0xdf9d), true);                
	m.Put(new Block(0x3000, 0x1000, 0xdf9e),true);               
	m.Put(new Block(0x4000, 0x1000, 0xdf9f),true);               
	m.Put(new Block(0x5000, 0xd4b, 0xdfa0),true);
	m.Put(new Block(0x52f5, 0xd0b, 0xdff3),true);
	m.Put(new Block(0x6000, 0x1000, 0xdff4),true);
	m.Put(new Block(0x6d67, 0x299, 0xdff8),true);
	m.Put(new Block(0x7000, 0x1000, 0xdff9),true);
	m.Put(new Block(0x7000, 0x1000, 0xdff5),true);
	m.Put(new Block(0x7d0f, 0x2f1, 0xe042),true);

	m.Put(new Block(0x8000, 0x1000, 0xe043),true);
	m.Put(new Block(0x8000, 0x1000, 0xdffa),true);
	m.Put(new Block(0x8000, 0x1000, 0xdff6),true);
	m.Put(new Block(0x884f, 0xd9, 0xe07b),true);
	m.Put(new Block(0x8b04, 0x4fc, 0xe07c),true);
	m.Put(new Block(0x9000, 0x1000, 0xe07d),true);
	m.Put(new Block(0x9000, 0x1000, 0xe044),true);
	m.Put(new Block(0x9000, 0x1000, 0xdffb),true);
	m.Put(new Block(0x9000, 0x8e7, 0xdff7),true);

	r = m.Get(0x8b00, block, length);

	BOOST_CHECK(block.offset == 0x8000);
	BOOST_CHECK(block.length == 0x1000);
	BOOST_CHECK(block.level == 0xe043);
	BOOST_CHECK(length == 0x4);

	r = m.Get(0x8b04, block, length);

	BOOST_CHECK(block.offset == 0x8b04);
	BOOST_CHECK(block.length == 0x4fc);
	BOOST_CHECK(length == 0x4fc);


	m.Put(new Block(0x9d35, 0x172, 0xdfd4),true);
	m.Put(new Block(0x9d94, 0x26c, 0xe04a),true);
	m.Put(new Block(0xa000, 0x1000, 0xe07e),true);
	m.Put(new Block(0xa000, 0x1000, 0xe04b),true);
	m.Put(new Block(0xa000, 0x1000, 0xe045),true);
	m.Put(new Block(0xa000, 0xea8, 0xdffc),true);
	m.Put(new Block(0xb000, 0x1000, 0xe07f),true);
	m.Put(new Block(0xb000, 0x1000, 0xe04c),true);
	m.Put(new Block(0xb000, 0x1000, 0xe046),true);
	m.Put(new Block(0xc000, 0x1000, 0xe080),true);
	m.Put(new Block(0xc000, 0x1000, 0xe04d),true);
	m.Put(new Block(0xc000, 0x1000, 0xe047),true);
	m.Put(new Block(0xd000, 0x1000, 0xe081),true);
	m.Put(new Block(0xd000, 0x1000, 0xe04e),true);
	m.Put(new Block(0xd000, 0x1000, 0xe048),true);
	m.Put(new Block(0xe000, 0x1000, 0xe082),true);
	m.Put(new Block(0xe000, 0x1000, 0xe04f),true);
	m.Put(new Block(0xe000, 0x2ca, 0xe049),true);
	m.Put(new Block(0xf000, 0x1000, 0xe083),true);
	m.Put(new Block(0xf000, 0x1000, 0xe050),true);
	m.Put(new Block(0x10000, 0x1000, 0xe084),true);
	m.Put(new Block(0x10000, 0x1000, 0xe051),true);
	m.Put(new Block(0x11000, 0x634, 0xe085),true);
	m.Put(new Block(0x11000, 0xdf6, 0xe052),true);

	r = m.Get(0x0, block, length);

	BOOST_CHECK(block.offset == 0x2a50);
	BOOST_CHECK(block.length == 0x5b0);
	BOOST_CHECK(block.level == 0xdf9d);
	BOOST_CHECK(length == 0x0);

	r = m.Get(0x1000, block, length);

	BOOST_CHECK(block.offset == 0x2a50);
	BOOST_CHECK(block.length == 0x5b0);
	BOOST_CHECK(block.level == 0xdf9d);
	BOOST_CHECK(length == 0x0);

	r = m.Get(0x2000, block, length);

	BOOST_CHECK(block.offset == 0x2a50);
	BOOST_CHECK(block.length == 0x5b0);
	BOOST_CHECK(block.level == 0xdf9d);
	BOOST_CHECK(length == 0x0);

	r = m.Get(0x3000, block, length);

	BOOST_CHECK(block.offset == 0x3000);
	BOOST_CHECK(block.length == 0x1000);
	BOOST_CHECK(block.level == 0xdf9e);
	BOOST_CHECK(length == 0x1000);


}

BOOST_AUTO_TEST_CASE(t8)
{
	LayerMap m;

	Block        block;
	off_t length;
	bool         r;


	m.Put(new Block(0x8000, 0x1000, 0x5e433), true);
	m.Put(new Block(0x8000, 0x1000, 0x5e3fe), true);
	m.Put(new Block(0x81ce, 0x3bb, 0x5e481), true);
	m.Put(new Block(0x8aea, 0x516, 0x5e464), true);
	m.Put(new Block(0x9000, 0x1000, 0x5e465), true);

	r = m.Get(0x8000, block, length);

	BOOST_CHECK(block.offset == 0x8000);
	BOOST_CHECK(block.length == 0x1000);
	BOOST_CHECK(block.level == 0x5e433);
	BOOST_CHECK(length == 0x1ce);

	r = m.Get(0x81ce, block, length);

	BOOST_CHECK(block.offset == 0x81ce);
	BOOST_CHECK(block.length == 0x3bb);
	BOOST_CHECK(length == 0x3bb);

	r = m.Get(0x8aea, block, length);

	BOOST_CHECK(block.offset == 0x8aea);
	BOOST_CHECK(block.length == 0x516);
	BOOST_CHECK(block.level == 0x5e464);
	BOOST_CHECK(length == 0x516);

	r = m.Get(0x8589, block, length);

	BOOST_CHECK(block.offset == 0x8000);
	BOOST_CHECK(block.length == 0x1000);
	BOOST_CHECK(block.level == 0x5e433);
	BOOST_CHECK(length == 0x561);

}

BOOST_AUTO_TEST_CASE(t9)
{
	LayerMap m;

	Block        block;
	off_t length;
	bool         r;

	m.Put(new Block(0x8, 0x12, 0x2e7), true);
	m.Put(new Block(0x4f, 0x234, 0x345), true);
	m.Put(new Block(0xbd, 0x805e, 0x368), true);
	m.Put(new Block(0x24b, 0x38, 0x34d), true);
	m.Put(new Block(0x1edf, 0x452c, 0x364), true);
	m.Put(new Block(0x24d9, 0x5c42, 0x365), true);

	r = m.Get(0x5000, block, length);

	BOOST_CHECK(block.offset == 0xbd);
	BOOST_CHECK(block.length == 0x805e);
	BOOST_CHECK(block.level == 0x368);
	BOOST_CHECK(length == 0x311b);

	r = m.Get(0x5447, block, length);

	BOOST_CHECK(block.offset == 0xbd);
	BOOST_CHECK(block.length == 0x805e);
	BOOST_CHECK(block.level == 0x368);
	BOOST_CHECK(length == 0x2cd4);
}

BOOST_AUTO_TEST_CASE(t10)
{
	LayerMap m;

	Block        block;
	off_t length;
	bool         r;

	m.Put(new Block(0x25, 0x26, 0x1), true);
	m.Put(new Block(0x2a3, 0xea1, 0x2), true);
	m.Put(new Block(0x1144, 0x1502, 0x3), true);
	m.Put(new Block(0x2646, 0x4bbc, 0x4), true);
	m.Put(new Block(0x7202, 0x4242, 0x5), true);
	m.Put(new Block(0xd1c9, 0x8cd8, 0x6), true);
	m.Put(new Block(0x2c83d, 0x6313, 0x7), true);
	m.Put(new Block(0x32b50, 0x872d, 0x8), true);
	m.Put(new Block(0x3ff88c, 0xe00, 0x9), true);

	r = m.Get(0xec000, block, length);

	BOOST_CHECK(block.offset == 0x3ff88c);
	BOOST_CHECK(block.length == 0xe00);
	BOOST_CHECK(block.level == 0x9);
	BOOST_CHECK(length == 0x0);

	r = m.Get(0xed000, block, length);

	BOOST_CHECK(block.offset == 0x3ff88c);
	BOOST_CHECK(block.length == 0xe00);
	BOOST_CHECK(block.level == 0x9);
	BOOST_CHECK(length == 0x0);
}

