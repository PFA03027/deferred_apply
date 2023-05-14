/**
 * @file deferred_apply.hpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-05-06
 *
 * @copyright Copyright (c) 2023, PFA03027@nifty.com
 *
 */

#ifndef DEFERRED_APPLY_EXP_HPP_
#define DEFERRED_APPLY_EXP_HPP_

#include <cxxabi.h>   // for abi::__cxa_demangle

#include <cstdlib>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef DEFERRED_APPLY_DEBUG
/* デバッグ用デマングル関数 */
inline char* demangle( const char* demangled )
{
	int status;
	return abi::__cxa_demangle( demangled, 0, 0, &status );
}
#endif

#if __cplusplus >= 201402L
// C++14以上の場合は、標準のインデックスシーケンスを使用する
template <size_t N>
using my_make_index_sequence = std::make_index_sequence<N>;
template <size_t... Is>
using my_index_sequence = std::index_sequence<Is...>;

#else

template <std::size_t...>
struct my_index_sequence {
};

namespace internal {
template <std::size_t I, std::size_t... Is>
struct S : S<I - 1, I - 1, Is...> {
};

template <std::size_t... Is>
struct S<0, Is...> {
	my_index_sequence<Is...> f();
};
}   // namespace internal

template <std::size_t N>
using my_make_index_sequence = decltype( internal::S<N> {}.f() );

#endif

////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 引数を保持するためのtuple用の型を求めるメタ関数の実装クラス
 *
 * Tがunion or クラスの場合で左辺値参照ならば、型Tのconst参照型を返す
 * Tが配列型ならば、型Tの要素型のconstへのポインタ型を返す（decay+add_constを適用する）
 * Tが上記以外なら（＝組込み型、enum型や、右辺値参照のクラス等）なら、型Tをそのまま返す。
 */
struct get_argument_store_type_impl {
	// 左辺値参照で、かつクラスかunion型の場合に、左辺値参照を型として返す。
	template <typename T>
	static auto check( T x ) -> typename std::enable_if<
		std::is_lvalue_reference<T>::value &&
			( std::is_class<typename std::remove_reference<T>::type>::value || std::is_union<typename std::remove_reference<T>::type>::value ),
		T>::type;

	// 配列型は、関数テンプレートと同じ推測を適用してた型に変換して返す。
	template <typename T>
	static auto check( T x ) -> typename std::enable_if<std::is_pointer<typename std::decay<T>::type>::value, typename std::decay<T>::type>::type;

	// 上記以外は、そのまま型を返す
	template <typename T>
	static auto check( ... ) -> typename std::remove_reference<T>::type;
};

/**
 * @brief 引数を保持するためのtuple用の型を求めるメタ関数
 */
template <typename T>
struct get_argument_store_type {
	using type = decltype( get_argument_store_type_impl::check<T>( std::declval<T>() ) );
};

/**
 * @brief get_argument_store_type<>で保持した実引数を、関数に適用するための型を求めるメタ関数の実装
 *
 */
struct get_argument_apply_type_impl {
	template <typename T, typename U>
	static auto check( T x, U y ) -> typename std::enable_if<std::is_rvalue_reference<T>::value, typename std::add_rvalue_reference<U>::type>::type;

	template <typename T, typename U>
	static auto check( T x, U y ) -> typename std::enable_if<std::is_lvalue_reference<T>::value, typename std::add_lvalue_reference<U>::type>::type;

	// 上記以外は、そのまま型を返す
	template <typename T, typename U>
	static auto check( ... ) -> U;
};

/**
 * @brief 引数を保持するためのtuple用の型を求めるメタ関数
 */

/**
 * @brief get_argument_store_type<>で保持した実引数を、関数に適用するための型を求めるメタ関数
 *
 * Tが右辺値参照の場合、Uを右辺値参照とする。
 * Tが左辺値参照の場合、Uを左辺値参照とする。
 * それ以外の場合は、Uをそのまま返す
 *
 * @tparam T もととなった引数の型
 * @tparam U 引数を保持するための型
 */
template <typename T, typename U>
struct get_argument_apply_type {
	using type = decltype( get_argument_apply_type_impl::check<T, U>( std::declval<T>(), std::declval<U>() ) );
};

////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 関数の実行を延期するために、一時的引数を保持することを目的としたクラス
 *
 * 一時的な保持を目的としているため、左辺値参照されたクラスは参照を保持する方式をとることで、コピーしない。
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。
 *
 * auto da = make_deferred_apply( a, b, ...);
 * auto ret = da.apply(f);
 *
 * da.apply(f)によって、f(a,b,...) が実行される。
 *
 * @note
 * C++11でもコンパイル可能なように、クラス内クラスの定義を先行定義するスタイルとしている。
 *
 * @note
 * ラムダ式のキャプチャを使うことでも同等のことが可能だが、
 * キャプチャ部分には右辺値を置けないため、左辺値として定義した変数＋参照キャプチャ経由で行う必要があるためかなり面倒。
 *
 * @note
 * std::tupleを関数引数に展開して関数を実行する技法は、一般的に使用されるパラメータパック展開の技法を適用することを期待している。
 * 実装例としては、deferred_apply を参照すること。
 *
 */
template <class... OrigArgs>
class deferred_apply {
public:
	deferred_apply( const deferred_apply& orig )
	  : values_( orig.values_ )
	{
	}
	deferred_apply( deferred_apply&& orig )
	  : values_( std::move( orig.values_ ) )
	{
	}

	template <typename XArgsHead,
	          typename... XArgs,
	          typename std::enable_if<!std::is_same<typename std::remove_reference<XArgsHead>::type, deferred_apply>::value>::type* = nullptr>
	deferred_apply( XArgsHead&& argshead, XArgs&&... args )
	  : values_( std::forward<XArgsHead>( argshead ), std::forward<XArgs>( args )... )
	{
#ifdef DEFERRED_APPLY_DEBUG
		printf( "Called constructor of deferred_apply\n" );
		printf( "\tXArgHead: %s, XArgs: %s\n", demangle( typeid( XArgsHead ).name() ), demangle( typeid( std::tuple<XArgs...> ).name() ) );
		printf( "\tvalues_: %s\n", demangle( typeid( values_ ).name() ) );
#endif
	}

	template <typename F>
	decltype( auto ) apply( F&& f )
#if __cpp_decltype_auto >= 201304
#else
		-> typename std::result_of<F( OrigArgs... )>::type
#endif
	{
#ifdef DEFERRED_APPLY_DEBUG
		printf( "apply_impl: %s\n", demangle( typeid( decltype( apply_impl( std::forward<F>( f ), my_make_index_sequence<std::tuple_size<tuple_args_t>::value>() ) ) ).name() ) );
#endif
		return apply_impl( std::forward<F>( f ), my_make_index_sequence<std::tuple_size<tuple_args_t>::value>() );
	}

private:
	template <typename F, size_t... Is>
	decltype( auto ) apply_impl( F&& f, my_index_sequence<Is...> )
#if __cpp_decltype_auto >= 201304
#else
		-> typename std::result_of<F( OrigArgs... )>::type
#endif
	{
#ifdef DEFERRED_APPLY_DEBUG
		printf( "f: %s\n", demangle( typeid( f ).name() ) );
#endif
		return ( std::forward<F>( f ) )(
			static_cast<typename get_argument_apply_type<OrigArgs, typename get_argument_store_type<OrigArgs>::type>::type>(
				std::get<Is>( values_ ) )... );
	}

	using tuple_args_t = std::tuple<typename get_argument_store_type<OrigArgs>::type...>;
	tuple_args_t values_;
};

// int printf_type( const char* p_tn )
// {
// 	return printf( "%s, ", p_tn );
// }

template <class... Args>
auto make_deferred_apply( Args&&... args ) -> decltype( deferred_apply<Args&&...>( std::forward<Args>( args )... ) )
{
#ifdef DEFERRED_APPLY_DEBUG
	// auto aa = { printf_type( demangle( typeid( Args ).name() ) )... };
	// printf( "\n" );
#endif
	return deferred_apply<Args&&...>( std::forward<Args>( args )... );
}

#endif
