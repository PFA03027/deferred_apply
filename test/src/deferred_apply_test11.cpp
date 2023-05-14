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

#include "gtest/gtest.h"

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

TEST( Deferred_Apply, test_integlal_literal )
{
	// Arrange
	auto xx = make_deferred_apply( 1 );

	// Act
	auto ret = xx.apply( t1_func );

	// Assert
	EXPECT_EQ( 1, ret );
}

TEST( Deferred_Apply, test_integlal_literal_const )
{
	// Arrange
	auto xx = make_deferred_apply( 1 );

	// Act
	auto ret = xx.apply( t2_func );

	// Assert
	EXPECT_EQ( 1, ret );
}

TEST( Deferred_Apply, test_pointer_to_integlal_literal )
{
	// Arrange
	int  test_int_data = 2;
	auto xx            = make_deferred_apply( &test_int_data );

	// Act
	auto ret = xx.apply( t3_func );

	// Assert
	EXPECT_EQ( &test_int_data, ret );
}

TEST( Deferred_Apply, test_pointer_to_integlal_literal_const )
{
	// Arrange
	const int test_int_data = 2;
	auto      xx            = make_deferred_apply( &test_int_data );

	// Act
	auto ret = xx.apply( t4_func );

	// Assert
	EXPECT_EQ( &test_int_data, ret );
}
