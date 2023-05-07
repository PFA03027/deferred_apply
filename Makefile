

all: test/deferred_apply_test_c++11.out test/deferred_apply_test.out
clean:
	-rm -f  test/deferred_apply_test_c++11.out test/deferred_apply_test.out

test: all
	test/deferred_apply_test.out
	echo
	test/deferred_apply_test_c++11.out

test/deferred_apply_test.out : test/deferred_apply_test.cpp inc/deferred_apply.hpp
	g++ -O2 -g -o test/deferred_apply_test.out -Iinc test/deferred_apply_test.cpp

test/deferred_apply_test_c++11.out : test/deferred_apply_test.cpp inc/deferred_apply.hpp
	g++ -O2 -g -std=c++11 -o test/deferred_apply_test_c++11.out -Iinc test/deferred_apply_test.cpp
