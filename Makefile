
SUFFIX   = .cpp
CFLAGS   = -Wall -O2 -DDEFERRED_APPLY_DEBUG

SRCDIR   = test
INCLUDE  = inc

SRCS  = $(wildcard $(SRCDIR)/*$(SUFFIX))

all: test/deferred_apply_test_c++11.out test/deferred_apply_test.out

clean:
	-rm -f  test/deferred_apply_test_c++11.out test/deferred_apply_test.out

test: all
	test/deferred_apply_test.out
	echo
	test/deferred_apply_test_c++11.out

test/deferred_apply_test.out : inc/deferred_apply.hpp $(SRCS)
	g++ $(CFLAGS) -o test/deferred_apply_test.out -I$(INCLUDE) $(SRCS)

test/deferred_apply_test_c++11.out : inc/deferred_apply.hpp $(SRCS)
	g++ $(CFLAGS) -std=c++11 -o test/deferred_apply_test_c++11.out -I$(INCLUDE) $(SRCS)

