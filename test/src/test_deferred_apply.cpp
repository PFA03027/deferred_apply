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

class testA {
public:
	testA( void )
	{
		printf( "%p: Called testA default-constructor\n", this );
	};
	testA( const testA& orig )
	{
		printf( "%p: Called testA copy-constructor\n", this );
	};
	testA( testA&& orig )
	{
		printf( "%p: Called testA move-constructor\n", this );
	};
	~testA()
	{
		printf( "%p: Called testA destructor\n", this );
	}
	testA& operator=( const testA& orig )
	{
		printf( "%p: Called testA copy-assingment\n", this );
		return *this;
	};
	testA& operator=( testA&& orig )
	{
		printf( "%p: Called testA move-assingment\n", this );
		return *this;
	};

	const char* c_str( void ) const
	{
		return "ThisIsTestA";
	}
};

////////////////////////

class printf_with_convert {
public:
	template <typename... OrigArgs>
	auto operator()( OrigArgs&&... args ) -> decltype( printf( std::forward<OrigArgs>( args )... ) )
	{
		return to_c_str_at_least( std::forward<OrigArgs>( args )... );
	}

private:
	/**
	 * @brief const char*を返すメンバ関数c_str()を呼び出せるかどうかを検査するメタ関数の実装
	 */
	struct is_callable_c_str_impl {
		template <typename T>
		static auto check( T* x ) -> decltype( std::declval<T>().c_str(), std::true_type() );

		template <typename T>
		static auto check( ... ) -> std::false_type;
	};

	/**
	 * @brief const char*を返すメンバ関数c_str()を呼び出せるかどうかを検査するメタ関数
	 */
	template <typename T>
	struct is_callable_c_str : decltype( is_callable_c_str_impl::check<typename std::remove_reference<T>::type>( nullptr ) ) {};

	template <typename T, typename std::enable_if<is_callable_c_str<T>::value>::type* = nullptr>
	static inline auto aaa( T& t ) -> decltype( t.c_str() )   // C++11でもコンパイルできるようにする。C++14なら、戻り値推論+decltype(auto)を使うのが良い
	{
		return t.c_str();
	}

	template <typename T, typename std::enable_if<!is_callable_c_str<T>::value>::type* = nullptr>
	static inline auto aaa( T& t ) -> T   // C++11でもコンパイルできるようにする。C++14なら、戻り値推論+decltype(auto)を使うのが良い
	{
		return t;
	}

	template <typename... OrigArgs>
	auto to_c_str_at_least( OrigArgs&&... args ) -> decltype( printf( std::forward<OrigArgs>( args )... ) )
	{
		return printf( aaa( args )... );
	}
};

TEST( Deferred_Apply, move_from_empty_instance )
{
	// Arrenge
	auto xx1 = make_deferred_apply( &printf, "l, %d, %s\n", 1, "m" );
	auto xx2 = std::move( xx1 );
	auto xx3 = std::move( xx1 );

	// Act
	// xx2.apply();

	// Assert
}

TEST( Deferred_Apply, test_apply_printf )
{
	// Arrenge
	int  ti  = 2;
	auto xx2 = make_deferred_apply( &printf, "l, %d, %d, %s\n", 1, ti, "m" );

	// Act
	xx2.apply();

	// Assert
}

TEST( Deferred_Apply, test_printf_with_convert2 )
{
	// Arrenge
	testA aa {};
	auto  xx3 = make_deferred_apply( printf_with_convert(), "n, %d, %s, %s, %s\n", 1, aa, testA {}, "o" );

	// Act
	xx3.apply();

	// Assert
}

TEST( Deferred_Apply, test_lreference_to_integlal )
{
	// Arrange
	struct local {
		static int& t_func( int& arg )
		{
			return arg;
		}
	};
	int  test_int_data  = 3;
	int  test_int_data2 = 5;
	auto xx             = make_deferred_apply( &local::t_func, test_int_data );
	int& ret2           = local::t_func( test_int_data2 );
	EXPECT_EQ( 5, ret2 );
	ret2 = 6;
	EXPECT_EQ( 6, test_int_data2 );

	// Act
	int& ret = xx.apply();

	// Assert
	static_assert( std::is_same<decltype( xx.apply() ), int&>::value, "ret should be left value reference" );
	EXPECT_EQ( 3, ret );
	ret = 4;
	EXPECT_EQ( 4, test_int_data );
}

TEST( Deferred_Apply_r, test_apply_printf )
{
	// Arrenge
	int  ti  = 2;
	auto xx2 = make_deferred_apply_r<uint8_t>( &printf, "l, %d, %d, %s\n", 1, ti, "m" );

	// Act
	xx2.apply();

	// Assert
}

TEST( Deferred_Apply_r, test_printf_with_convert2 )
{
	// Arrenge
	testA aa {};
	auto  xx3 = make_deferred_apply_r<uint8_t>( printf_with_convert(), "n, %d, %s, %s, %s\n", 1, aa, testA {}, "o" );

	// Act
	xx3.apply();

	// Assert
}

TEST( Deferred_Apply_r, copy_constructor )
{
	// Arrenge
	testA aa {};
	auto  xx3 = make_deferred_apply_r<uint8_t>( printf_with_convert(), "n, %d, %s, %s, %s\n", 1, aa, testA {}, "o" );
	auto  xx4 = xx3;

	// Act
	xx4.apply();

	// Assert
}

TEST( Deferred_Apply_r, move_constructor )
{
	// Arrenge
	testA aa {};
	auto  xx3 = make_deferred_apply_r<uint8_t>( printf_with_convert(), "n, %d, %s, %s, %s\n", 1, aa, testA {}, "o" );
	auto  xx4 = std::move( xx3 );

	// Act
	xx4.apply();

	// Assert
}

class void_functor {
public:
	void_functor( void )                = default;
	void_functor( const void_functor& ) = default;
	void_functor( void_functor&& )      = default;
	~void_functor()                     = default;

	template <typename... OrigArgs>
	void operator()( OrigArgs&&... args )
	{
		return;
	}

private:
	std::shared_ptr<int> up_tmp_;
};

TEST( Deferred_Apply_r, big_arguments )
{
	// Arrenge
	testA aa {};
	auto  xx3 = make_deferred_apply_r<void>(
        void_functor(),
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        0,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
        32,
        33 );

	// Act
	xx3.apply();
	auto xx4 = xx3;
	auto xx5 = std::move( xx3 );
	xx4.apply();
	xx5.apply();

	// Assert
}

TEST( Deferred_Apply, CanCall_apply_functor )
{
	// Arrange
	struct local {
		const char* operator()( const char* arg )
		{
			call_counter++;
			return arg;
		}

		int call_counter = 0;
	};
	local aa;
	auto  xx = make_deferred_apply( aa, "test_string" );

	// Act
	xx.apply();
	auto ret = xx.apply();

	// Assert
	EXPECT_EQ( 2, aa.call_counter );
	EXPECT_STREQ( "test_string", ret );
}
