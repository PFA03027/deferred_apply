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
////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief タプルの要素を関数の引数に適用する際の型を求めるメタ関数の実装クラス
 */
struct get_argument_apply_type_impl {
	// 左辺値参照で、かつクラスかunion型の場合に、const左辺値参照を型として返す。
	template <typename T>
	static auto check( T x ) -> typename std::enable_if<is_callable_c_str<T>::value, decltype( x.c_str() )>::type;

	// 上記以外は、そのまま型を返す
	template <typename T>
	static auto check( ... ) -> typename std::remove_reference<T>::type;
};

/**
 * @brief タプルの要素を関数の引数に適用する際の型を求めるメタ関数
 */
template <typename T>
struct get_argument_apply_type {
	using type = decltype( get_argument_apply_type_impl::check<T>( std::declval<T>() ) );
};

template <size_t I, typename Tuple, typename std::enable_if<is_callable_c_str<typename std::tuple_element<I, typename std::remove_reference<Tuple>::type>::type>::value>::type* = nullptr>
static auto get_argument_apply_value( Tuple& t ) -> decltype( std::get<I>( t ).c_str() )   // C++11でもコンパイルできるようにする。C++14なら、戻り値推論+decltype(auto)を使うのが良い
{
	return std::get<I>( t ).c_str();
}

template <size_t I, typename Tuple, typename std::enable_if<!is_callable_c_str<typename std::tuple_element<I, typename std::remove_reference<Tuple>::type>::type>::value>::type* = nullptr>
static auto get_argument_apply_value( Tuple& t ) -> decltype( std::get<I>( t ) )   // C++11でもコンパイルできるようにする。C++14なら、戻り値推論+decltype(auto)を使うのが良い
{
	return std::get<I>( t );
}

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
// テンプレートテンプレートを使った遅延適用の習作

template <typename R,
          template <typename XR, typename XTuple, size_t N> typename F,
          typename... Args>
class TTtest {
public:
	TTtest( Args&&... args )
	  : args_tuple_( std::forward<Args>( args )... )
	{
	}

	R apply( void )
	{
		return f_( args_tuple_ );
	}

private:
	using tuple_args_t = std::tuple<Args...>;
	tuple_args_t                          args_tuple_;
	F<R, tuple_args_t, sizeof...( Args )> f_;
};

template <typename R, typename Tuple, size_t N>
class dummy_printf {
public:
	R operator()( Tuple& arg_tuple )
	{
		return apply( arg_tuple, my_make_index_sequence<N>() );
	}

	template <size_t... Is>
	R apply( Tuple& arg_tuple, my_index_sequence<Is...> )
	{
		return printf( std::get<Is>( arg_tuple )... );
	}
};

////////////////////////

template <typename Tuple, size_t N>
class printf_with_convert {
	template <size_t... Is>
	auto apply( Tuple& arg_tuple, my_index_sequence<Is...> ) -> int
	{
		using cur_apply_tuple_t = std::tuple<typename get_argument_apply_type<typename std::tuple_element<Is, Tuple>::type>::type...>;
		cur_apply_tuple_t apply_values( get_argument_apply_value<Is>( arg_tuple )... );
		printf( "apply_values: %s\n", demangle( typeid( cur_apply_tuple_t ).name() ) );
		// printf( std::get<Is>( apply_values )... );
		return printf( std::get<Is>( apply_values )... );
	}

public:
	auto operator()( Tuple& arg_tuple ) -> decltype( apply( arg_tuple, my_make_index_sequence<N>() ) )
	{
		return apply( arg_tuple, my_make_index_sequence<N>() );
	}
};

int main( void )
{
	static_assert( is_callable_c_str<testA const&>::value );

	testA aa {};

	auto xx = TTtest<int, dummy_printf, const char*, const char*>( "%s\n", "test" );
	xx.apply();

	auto xx2 = deferred_apply<int, printf_with_convert>( "l, %d, %s, %s, %s\n", 1, aa, testA {}, "m" );
	xx2.apply();

	return EXIT_SUCCESS;
}
