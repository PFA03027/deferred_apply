/**
 * @file deferred_apply_test.cpp
 * @author PFA03027@nifty.com
 * @brief 引数情報を保持するクラスの実現方法を検討する
 * @version 0.1
 * @date 2023-05-05
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "deferred_apply.hpp"

int t1_func( int arg )
{
	return arg;
}

int t2_func( const int arg )
{
	return arg;
}

int* t3_func( int* arg )
{
	return arg;
}

const int* t4_func( const int* arg )
{
	return arg;
}

int test1( void )
{
	int test_int_data = 2;
	{
		auto xx = make_deferred_apply( 1 );
		if ( 1 != xx.apply( t1_func ) ) {
			return EXIT_FAILURE;
		}
	}
	{
		auto xx = make_deferred_apply( 1 );
		if ( 1 != xx.apply( t2_func ) ) {
			return EXIT_FAILURE;
		}
	}
	{
		auto xx = make_deferred_apply( &test_int_data );
		if ( ( &test_int_data ) != xx.apply( t3_func ) ) {
			return EXIT_FAILURE;
		}
	}
	{
		auto xx = make_deferred_apply( &test_int_data );
		if ( ( &test_int_data ) != xx.apply( t4_func ) ) {
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
