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

TEST( Deferred_Apply, test_printf_with_convert1 )
{
	// Arrenge
	auto xx2 = make_deferred_apply( "l, %d, %s\n", 1, "m" );

	// Act
	xx2.apply( &printf );

	// Assert
}

TEST( Deferred_Apply, test_printf_with_convert2 )
{
	// Arrenge
	testA aa {};
	auto  xx3 = make_deferred_apply( "n, %d, %s, %s, %s\n", 1, aa, testA {}, "o" );

	// Act
	xx3.apply( printf_with_convert() );

	// Assert
}
