#ifndef ASSERT_H
#define ASSERT_H

#include <rlog/rlog.h>
#include <stdlib.h>

#define assert(eval)								\
	if (!(eval)) {								\
		rError("ASSERT %s (%s)", __PRETTY_FUNCTION__, __STRING(eval));	\
		abort();							\
	}

#endif

