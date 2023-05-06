

all: test/deferred_apply_test.out

test: all
	test/deferred_apply_test.out

test/deferred_apply_test.out : test/deferred_apply_test.cpp inc/deferred_apply.hpp
	g++ -o test/deferred_apply_test.out -Iinc test/deferred_apply_test.cpp
