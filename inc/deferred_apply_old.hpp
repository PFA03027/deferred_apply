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

#ifndef DEFERRED_APPLY_HPP_
#define DEFERRED_APPLY_HPP_

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
		typename std::add_lvalue_reference<typename std::remove_reference<T>::type>::type>::type;

	// 配列型は、関数テンプレートと同じ推測を適用してconstへのポインタ型変換して返す。
	template <typename T>
	static auto check( T x ) -> typename std::enable_if<
		std::is_pointer<typename std::decay<T>::type>::value,
		typename std::add_const<typename std::decay<T>::type>::type>::type;

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

////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 関数の実行を延期するために、一時的引数を保持することを目的としたクラス
 *
 * 一時的な保持を目的としているため、左辺値参照されたクラスは参照を保持する方式をとることで、コピーしない。
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。
 *
 * @tparam R apply()の戻り値型。テンプレート関数Fの戻り値型から変換可能な型であること。
 * @tparam F apply()が呼ばれたときに、コンストラクタで受け取った引数を引き渡す先となるテンプレート関数。
 * @tparam XTuple テンプレート関数Fの第1テンプレート引数。引数を保持するためのテンプレート型で定義されたstd::tuple
 *
 * テンプレート関数Fは、下記の関数呼び出しオペレータを定義すること。
 * deferred_apply_tuple_func::apply()は、この関数オペレータを呼び出す。
 * @code {.cpp}

    R operator()( Tuple& arg_tuple )
    {
        ...
    }

 * @endcode
 *
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
template <typename R, template <typename XTuple, typename... OrigArgs> typename F>
class deferred_apply_tuple_func {
	constexpr static size_t buff_size = 256;

	////////////////////////////////////////////////////////////////////////////////////////////
	class deferred_apply_carrier_base {
	public:
		virtual ~deferred_apply_carrier_base()                                               = default;
		virtual deferred_apply_carrier_base*                 placement_new_copy( void* ptr ) = 0;
		virtual deferred_apply_carrier_base*                 placement_new_move( void* ptr ) = 0;
		virtual std::unique_ptr<deferred_apply_carrier_base> make_clone( void )              = 0;
		virtual R                                            apply_func( void )              = 0;
	};

	template <class... OrigArgs>
	class deferred_tt_apply_carrier : public deferred_apply_carrier_base {
	public:
		template <class XArgHead, class... XArgs, typename std::enable_if<!std::is_same<typename std::remove_reference<XArgHead>::type, deferred_tt_apply_carrier>::value>::type* = nullptr>
		deferred_tt_apply_carrier( XArgHead&& arghead, XArgs&&... args )
		  : values_( std::forward<XArgHead>( arghead ), std::forward<XArgs>( args )... )
		  , f_()
		{
#ifdef DEFERRED_APPLY_DEBUG
			printf( "Called constructor of deferred_apply_carrier\n" );
			printf( "\tXArgHead: %s, XArgs: %s\n", demangle( typeid( XArgHead ).name() ), demangle( typeid( std::tuple<XArgs...> ).name() ) );
			printf( "\tvalues_: %s\n", demangle( typeid( values_ ).name() ) );
#endif
		}
		deferred_tt_apply_carrier( const deferred_tt_apply_carrier& orig )
		  : values_( orig.values_ )
		  , f_()
		{
#ifdef DEFERRED_APPLY_DEBUG
			printf( "Called copy-constructor of deferred_tt_apply_carrier\n" );
#endif
		}
		deferred_tt_apply_carrier( deferred_tt_apply_carrier&& orig )
		  : values_( std::move( orig.values_ ) )
		  , f_()
		{
#ifdef DEFERRED_APPLY_DEBUG
			printf( "Called move-constructor of deferred_tt_apply_carrier\n" );
#endif
		}

		~deferred_tt_apply_carrier()
		{
#ifdef DEFERRED_APPLY_DEBUG
			printf( "Called destructor of deferred_tt_apply_carrier\n" );
#endif
		}

		deferred_apply_carrier_base* placement_new_copy( void* ptr ) override
		{
			return new ( ptr ) deferred_tt_apply_carrier( *this );
		}
		deferred_apply_carrier_base* placement_new_move( void* ptr ) override
		{
			return new ( ptr ) deferred_tt_apply_carrier( std::move( *this ) );
		}
		std::unique_ptr<deferred_apply_carrier_base> make_clone( void ) override
		{
#if __cpp_lib_make_unique >= 201304
			return std::make_unique<deferred_tt_apply_carrier>( *this );
#else
			return std::unique_ptr<deferred_apply_carrier_base>( new deferred_tt_apply_carrier( std::move( *this ) ) );
#endif
		}

		R apply_func( void ) override
		{
			static_assert( std::is_same<decltype( f_( values_ ) ), R>::value, "f_() return value type should be convertible to R" );
			return f_( values_ );
		}

	private:
		using tuple_args_t = std::tuple<typename get_argument_store_type<OrigArgs>::type...>;
		tuple_args_t                 values_;
		F<tuple_args_t, OrigArgs...> f_;
	};

public:
	deferred_apply_tuple_func( const deferred_apply_tuple_func& orig )
	  : up_args_( nullptr )
	  , p_args_( nullptr )
	{
		if ( orig.up_args_ != nullptr ) {
			up_args_ = orig.up_args_->make_clone();
			p_args_  = up_args_.get();
			return;
		}
		p_args_ = orig.p_args_->placement_new_copy( placement_new_buffer );
	}
	deferred_apply_tuple_func( deferred_apply_tuple_func&& orig )
	  : up_args_( std::move( orig.up_args_ ) )
	  , p_args_( nullptr )
	{
		if ( up_args_ != nullptr ) {
			p_args_      = up_args_.get();
			orig.p_args_ = nullptr;
			return;
		}

		if ( orig.p_args_ == nullptr ) {
			return;
		}

		p_args_ = orig.p_args_->placement_new_move( placement_new_buffer );
		orig.p_args_->~deferred_apply_carrier_base();
		orig.p_args_ = nullptr;
	}

#if __cpp_if_constexpr >= 201606
	template <class ArgHead,
	          class... OrigArgs,
	          typename std::enable_if<!std::is_same<ArgHead, deferred_apply_tuple_func>::value>::type* = nullptr>
	deferred_apply_tuple_func( ArgHead&& arghead, OrigArgs&&... args )
	  : up_args_( nullptr )
	  , p_args_( nullptr )
	{
		using cur_deferred_apply_t = deferred_tt_apply_carrier<ArgHead, OrigArgs...>;
#ifdef DEFERRED_APPLY_DEBUG
		printf( "cur_deferred_apply_t: %s, size=%zu\n", demangle( typeid( cur_deferred_apply_t ).name() ), sizeof( cur_deferred_apply_t ) );
#endif

		if constexpr ( buff_size < sizeof( cur_deferred_apply_t ) ) {   // 本来は、C++17から導入されたif constexpr構文を使用するのがあるべき姿
			up_args_ = std::make_unique<cur_deferred_apply_t>( std::forward<ArgHead>( arghead ), std::forward<OrigArgs>( args )... );
			p_args_  = up_args_.get();
		} else {
			p_args_ = new ( placement_new_buffer ) cur_deferred_apply_t( std::forward<ArgHead>( arghead ), std::forward<OrigArgs>( args )... );
		}
	}
#else   // __cpp_if_constexpr
	template <class ArgHead,
	          class... OrigArgs,
	          typename std::enable_if<!std::is_same<ArgHead, deferred_apply_tuple_func>::value && ( buff_size < sizeof( deferred_tt_apply_carrier<ArgHead, OrigArgs...> ) )>::type* = nullptr>
	deferred_apply_tuple_func( ArgHead&& arghead, OrigArgs&&... args )
	  : up_args_( nullptr )
	  , p_args_( nullptr )
	{
		using cur_deferred_apply_t = deferred_tt_apply_carrier<ArgHead, OrigArgs...>;
#ifdef DEFERRED_APPLY_DEBUG
		printf( "cur_deferred_apply_t: %s, size=%zu\n", demangle( typeid( cur_deferred_apply_t ).name() ), sizeof( cur_deferred_apply_t ) );
#endif

#if __cpp_lib_make_unique >= 201304
		up_args_ = std::make_unique<cur_deferred_apply_t>( std::forward<ArgHead>( arghead ), std::forward<OrigArgs>( args )... );
#else
		up_args_ = std::unique_ptr<cur_deferred_apply_t>( new cur_deferred_apply_t( std::forward<ArgHead>( arghead ), std::forward<OrigArgs>( args )... ) );
#endif
		p_args_  = up_args_.get();
	}

	template <class ArgHead,
	          class... OrigArgs,
	          typename std::enable_if<!std::is_same<ArgHead, deferred_apply_tuple_func>::value && ( buff_size >= sizeof( deferred_tt_apply_carrier<ArgHead, OrigArgs...> ) )>::type* = nullptr>
	deferred_apply_tuple_func( ArgHead&& arghead, OrigArgs&&... args )
	  : up_args_( nullptr )
	  , p_args_( nullptr )
	{
		using cur_deferred_apply_t = deferred_tt_apply_carrier<ArgHead, OrigArgs...>;
#ifdef DEFERRED_APPLY_DEBUG
		printf( "cur_deferred_apply_t: %s, size=%zu\n", demangle( typeid( cur_deferred_apply_t ).name() ), sizeof( cur_deferred_apply_t ) );
#endif

		p_args_ = new ( placement_new_buffer ) cur_deferred_apply_t( std::forward<ArgHead>( arghead ), std::forward<OrigArgs>( args )... );
	}
#endif   // __cpp_if_constexpr

	~deferred_apply_tuple_func()
	{
		if ( up_args_ != nullptr ) return;
		if ( p_args_ == nullptr ) return;

		p_args_->~deferred_apply_carrier_base();
	}

	R apply( void )
	{
		return p_args_->apply_func();
	}

private:
	std::unique_ptr<deferred_apply_carrier_base> up_args_;
	deferred_apply_carrier_base*                 p_args_;
	char                                         placement_new_buffer[buff_size];
};

#endif
