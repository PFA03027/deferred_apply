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
#include <memory>
#include <typeindex>

#include "deferred_apply.hpp"

#include "gtest/gtest.h"

TEST( Deferred_Apply, test_integlal_literal )
{
	// Arrange
	struct local {
		static int t_func( int arg )
		{
			return arg;
		}
	};
	auto xx = make_deferred_apply( 1 );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( 1, ret );
}

TEST( Deferred_Apply, test_integlal_literal_const )
{
	// Arrange
	struct local {
		static int t_func( const int arg )
		{
			return arg;
		}
	};
	auto xx = make_deferred_apply( 1 );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( 1, ret );
}

TEST( Deferred_Apply, test_pointer_to_integlal_literal )
{
	// Arrange
	struct local {
		static int* t_func( int* arg )
		{
			return arg;
		}
	};
	int  test_int_data = 2;
	auto xx            = make_deferred_apply( &test_int_data );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( &test_int_data, ret );
}

TEST( Deferred_Apply, test_pointer_to_integlal_literal_const )
{
	// Arrange
	struct local {
		static const int* t_func( const int* arg )
		{
			return arg;
		}
	};
	const int test_int_data = 2;
	auto      xx            = make_deferred_apply( &test_int_data );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( &test_int_data, ret );
}

TEST( Deferred_Apply, test_lreference_to_integlal_literal )
{
	// Arrange
	struct local {
		static int& t_func( int& arg )
		{
			return arg;
		}
	};
	int  test_int_data = 3;
	auto xx            = make_deferred_apply( test_int_data );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	// static_assert( std::is_lvalue_reference<decltype( xx.apply( t5_func ) )>::value, "ret should be left value reference" );
#ifdef DEFERRED_APPLY_DEBUG
	printf( "xx.apply( local::t_func ): %s\n", demangle( typeid( decltype( xx.apply( local::t_func ) ) ).name() ) );
#endif
	EXPECT_EQ( 3, ret );
	ret = 4;
	// EXPECT_EQ( 4, test_int_data );
}

TEST( Deferred_Apply, test_lreference_to_integlal_literal_const )
{
	// Arrange
	struct local {
		static const int& t_func( const int& arg )
		{
			return arg;
		}
	};
	const int test_int_data = 3;
	auto      xx            = make_deferred_apply( test_int_data );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( std::type_index( typeid( const int& ) ), std::type_index( typeid( ret ) ) );
	// static_assert( std::is_lvalue_reference<decltype( xx.apply( t6_func ) )>::value, "ret should be left value reference" );
	// EXPECT_EQ( &test_int_data, &ret );
	EXPECT_EQ( test_int_data, ret );
}

TEST( Deferred_Apply, test_move_only )
{
	// Arrange
	struct local {
		static std::unique_ptr<int> t_func( std::unique_ptr<int> arg )
		{
			return arg;
		}
	};
	std::unique_ptr<int> up_data = std::unique_ptr<int>( new int( 5 ) );
	int*                 p_addr  = up_data.get();
	auto                 xx      = make_deferred_apply( std::move( up_data ) );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( std::type_index( typeid( up_data ) ), std::type_index( typeid( ret ) ) );
	EXPECT_EQ( p_addr, ret.get() );
	EXPECT_EQ( 5, ( *ret ) );
}

TEST( Deferred_Apply, test_literal_string )
{
	// Arrange
	struct local {
		static const char* t_func( const char* arg )
		{
			return arg;
		}
	};
	auto xx = make_deferred_apply( "test_string" );

	// Act
	auto ret = xx.apply( local::t_func );

	// Assert
	EXPECT_EQ( std::type_index( typeid( const char* ) ), std::type_index( typeid( ret ) ) );
	EXPECT_STREQ( "test_string", ret );
}