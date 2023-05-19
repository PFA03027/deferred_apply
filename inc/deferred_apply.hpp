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

#include <cxxabi.h>   // for abi::__cxa_deferred_apply_internal::demangle

#include <cstdlib>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace deferred_apply_internal {

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
// C++11の場合は、代替インデックスシーケンスを使用する
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

	// 上記以外は、そのまま型を返す。
	template <typename T>
	static auto check( ... ) -> typename std::remove_reference<T>::type;
};

/**
 * @brief 引数を保持するためのtuple用の型を求めるメタ関数
 *
 * @li Tがunion or クラスの場合で左辺値参照ならば、型Tの左辺値参照型を返す
 * @li Tが配列型ならば、型Tの要素型のポインタ型を返す（decayを適用する）
 * @li Tが上記以外なら（＝組込み型、enum型や、右辺値参照のクラス等）なら、型Tをそのまま返す。
 */
template <typename T>
struct get_argument_store_type {
	using type = decltype( get_argument_store_type_impl::check<T>( std::declval<T>() ) );
};

/**
 * @brief deferred_apply_internal::get_argument_store_type<>で保持した実引数を、関数に適用するための型を求めるメタ関数の実装
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
 * @brief deferred_apply_internal::get_argument_store_type<>で保持した実引数を、関数に適用するための型を求めるメタ関数
 *
 * Tが右辺値参照の場合、Uの右辺値参照型を返す。
 * Tが左辺値参照の場合、Uの左辺値参照型を返す。
 * それ以外の場合は、Uをそのまま返す
 *
 * @tparam T もととなった引数の型
 * @tparam U 引数を保持するための型
 */
template <typename T, typename U>
struct get_argument_apply_type {
	using type = decltype( get_argument_apply_type_impl::check<T, U>( std::declval<T>(), std::declval<U>() ) );
};

}   // namespace deferred_apply_internal

////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 関数の実行を延期するために、一時的引数を保持することを目的としたクラス
 *
 * 一時的な保持を目的としているため、左辺値参照されたクラスは参照を保持する方式で、コピーしない。
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。
 *
 * 使用例：
 * auto da = make_deferred_applying_arguments( a, b, ... );
 * auto ret = da.apply(f);
 *
 * da.apply(f)によって、f(a,b,...) が実行される。
 *
 * apply()の再適用が可能かどうかは、引数をどのように渡したか、あるいはfの特性に依存する。
 * 例えば、aが右辺値参照で引き渡された場合、1回目のapply()の適用によって、deferred_apply内で保持していた値が無効値となっている可能性がある。
 *
 * @note
 * ラムダ式のキャプチャを使うことでも同等のことが可能だが、
 * キャプチャ部分には右辺値を置けないため、左辺値として定義した変数＋参照キャプチャ経由で行う必要があるためかなり面倒。
 *
 */
template <typename... OrigArgs>
class deferred_applying_arguments {
public:
	deferred_applying_arguments( const deferred_applying_arguments& orig )
	  : values_( orig.values_ )
	{
	}
	deferred_applying_arguments( deferred_applying_arguments&& orig )
	  : values_( std::move( orig.values_ ) )
	{
	}

	template <typename XArgsHead,
	          typename... XArgs,
	          typename std::enable_if<!std::is_same<typename std::remove_reference<XArgsHead>::type, deferred_applying_arguments>::value>::type* = nullptr>
	deferred_applying_arguments( XArgsHead&& argshead, XArgs&&... args )
	  : values_( std::forward<XArgsHead>( argshead ), std::forward<XArgs>( args )... )
	{
#ifdef DEFERRED_APPLY_DEBUG
		printf( "Called constructor of deferred_applying_arguments\n" );
		printf( "\this class: %s\n", deferred_apply_internal::demangle( typeid( *this ).name() ) );
		printf( "\tXArg: %s\n", deferred_apply_internal::demangle( typeid( std::tuple<XArgsHead, XArgs...> ).name() ) );
		printf( "\tvalues_: %s\n", deferred_apply_internal::demangle( typeid( values_ ).name() ) );
#endif
	}

	template <typename F>
#if __cpp_decltype_auto >= 201304
	decltype( auto ) apply( F&& f )
#else
	auto apply( F&& f ) -> typename std::result_of<F( OrigArgs... )>::type
#endif
	{
#ifdef DEFERRED_APPLY_DEBUG
		printf( "apply_impl: %s\n", deferred_apply_internal::demangle( typeid( decltype( apply_impl( std::forward<F>( f ), deferred_apply_internal::my_make_index_sequence<std::tuple_size<tuple_args_t>::value>() ) ) ).name() ) );
#endif
		return apply_impl( std::forward<F>( f ), deferred_apply_internal::my_make_index_sequence<std::tuple_size<tuple_args_t>::value>() );
	}

private:
	template <typename F, size_t... Is>
#if __cpp_decltype_auto >= 201304
	decltype( auto ) apply_impl( F&& f, deferred_apply_internal::my_index_sequence<Is...> )
#else
	auto apply_impl( F&& f, deferred_apply_internal::my_index_sequence<Is...> ) -> typename std::result_of<F( OrigArgs... )>::type
#endif
	{
#ifdef DEFERRED_APPLY_DEBUG
		printf( "f: %s\n", deferred_apply_internal::demangle( typeid( f ).name() ) );
#endif
		return f(
			static_cast<typename deferred_apply_internal::get_argument_apply_type<OrigArgs, typename deferred_apply_internal::get_argument_store_type<OrigArgs>::type>::type>(
				std::get<Is>( values_ ) )... );
	}

	using tuple_args_t = std::tuple<typename deferred_apply_internal::get_argument_store_type<OrigArgs>::type...>;
	tuple_args_t values_;
};

template <class... Args>
auto make_deferred_applying_arguments( Args&&... args ) -> decltype( deferred_applying_arguments<Args&&...>( std::forward<Args>( args )... ) )
{
#ifdef DEFERRED_APPLY_DEBUG
	// auto aa = { printf_type( deferred_apply_internal::demangle( typeid( Args ).name() ) )... };
	// printf( "\n" );
#endif
	return deferred_applying_arguments<Args&&...>( std::forward<Args>( args )... );
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename R>
class deferred_apply_base {
public:
	virtual ~deferred_apply_base()                                               = default;
	virtual R                                    apply_func( void )              = 0;
	virtual deferred_apply_base*                 placement_new_copy( void* ptr ) = 0;
	virtual deferred_apply_base*                 placement_new_move( void* ptr ) = 0;
	virtual std::unique_ptr<deferred_apply_base> make_copy_clone( void )         = 0;
};

/**
 * @brief 引数と関数を保持するためのクラス
 *
 * @tparam R Fの戻り値の型
 * @tparam F 関数、あるいは関数オブジェクトの型
 * @tparam OrigArgs Fに適用する引数の型
 */
template <typename R, typename F, typename... OrigArgs>
class deferred_apply_container : public deferred_apply_base<R> {
public:
	deferred_apply_container( const deferred_apply_container& orig )
	  : functor_( orig.functor_ )
	  , arguments_keeper_( orig.arguments_keeper_ )
	{
	}
	deferred_apply_container( deferred_apply_container&& orig )
	  : functor_( std::move( orig.functor_ ) )
	  , arguments_keeper_( std::move( orig.arguments_keeper_ ) )
	{
	}

	template <typename XF,
	          typename... XArgs,
	          typename std::enable_if<!std::is_same<typename std::remove_reference<XF>::type, deferred_apply_container>::value>::type* = nullptr>
	deferred_apply_container( XF&& f, XArgs&&... args )
	  : functor_( std::forward<XF>( f ) )
	  , arguments_keeper_( std::forward<XArgs>( args )... )
	{
	}

	R apply_func( void ) override
	{
		return arguments_keeper_.apply( functor_ );
	}

	deferred_apply_base<R>* placement_new_copy( void* ptr ) override
	{
		return new ( ptr ) deferred_apply_container( *this );
	}
	deferred_apply_base<R>* placement_new_move( void* ptr ) override
	{
		return new ( ptr ) deferred_apply_container( std::move( *this ) );
	}
	std::unique_ptr<deferred_apply_base<R>> make_copy_clone( void ) override
	{
#if __cpp_lib_make_unique >= 201304
		return std::make_unique<deferred_apply_container>( *this );
#else
		return std::unique_ptr<deferred_apply_base<R>>( new deferred_apply_container( *this ) );
#endif
	}

private:
	F                                        functor_;
	deferred_applying_arguments<OrigArgs...> arguments_keeper_;
};

/**
 * @brief 関数の実行を延期するために、関数と引数を保持することを目的としたクラス
 *
 * 一時的な保持を目的としているため、左辺値参照されたクラスは参照を保持する方式で、コピーしない。
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。
 *
 * 使用例：
 * auto da = make_deferred_apply( f, a, b, ... );
 * auto ret = da.apply();
 *
 * da.apply(f)によって、f(a,b,...) が実行される。
 *
 * apply()の再適用が可能かどうかは、引数をどのように渡したか、あるいはfの特性に依存する。
 * 例えば、aが右辺値参照で引き渡された場合、1回目のapply()の適用によって、deferred_apply内で保持していた値が無効値となっている可能性がある。
 *
 * apply()のための戻り値の型がテンプレート型である以外は、型情報が隠されているため、メンバ変数定義も容易になる。
 * 一方で、deferred_applying_arguments<>とは異なり、動的に適用する関数fを変更することはできない。
 *
 * @tparam R メンバ関数apply()の戻り値の型
 */
template <typename R>
class deferred_apply {
	constexpr static size_t buff_size = 128;

public:
	deferred_apply( const deferred_apply& orig )
	  : up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		if ( orig.up_cntner_ != nullptr ) {
			up_cntner_ = orig.up_cntner_->make_copy_clone();
			p_cntner_  = up_cntner_.get();
		} else {
			p_cntner_ = orig.p_cntner_->placement_new_copy( placement_new_buffer );
		}
		return;
	}
	deferred_apply( deferred_apply&& orig )
	  : up_cntner_( std::move( orig.up_cntner_ ) )
	  , p_cntner_( nullptr )
	{
		if ( up_cntner_ != nullptr ) {
			p_cntner_      = up_cntner_.get();
			orig.p_cntner_ = nullptr;
		} else if ( orig.p_cntner_ == nullptr ) {
			// orig is empty. Therefore, nothing to do
		} else {
			p_cntner_ = orig.p_cntner_->placement_new_move( placement_new_buffer );
			orig.p_cntner_->~deferred_apply_base();
			orig.p_cntner_ = nullptr;
		}
		return;
	}

#if __cpp_if_constexpr >= 201606
	template <typename F,
	          typename... Args,
	          typename std::enable_if<!std::is_same<F, deferred_apply>::value>::type* = nullptr>
	deferred_apply( F&& f, Args&&... args )
	  : up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		using cur_container_t = deferred_apply_container<R, F, Args&&...>;

		if constexpr ( buff_size < sizeof( cur_container_t ) ) {   // 本来は、C++17から導入されたif constexpr構文を使用するのがあるべき姿
			up_cntner_ = std::make_unique<cur_container_t>( std::forward<F>( f ), std::forward<Args>( args )... );
			p_cntner_  = up_cntner_.get();
		} else {
			p_cntner_ = new ( placement_new_buffer ) cur_container_t( std::forward<F>( f ), std::forward<Args>( args )... );
		}
	}
#else   // __cpp_if_constexpr
	template <typename F,
	          typename... Args,
	          typename std::enable_if<!std::is_same<F, deferred_apply>::value && ( buff_size < sizeof( deferred_apply_container<R, F, Args...> ) )>::type* = nullptr>
	deferred_apply( F&& f, Args&&... args )
	  : up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		using cur_container_t = deferred_apply_container<R, F, Args&&...>;

#if __cpp_lib_make_unique >= 201304
		up_cntner_            = std::make_unique<cur_container_t>( std::forward<F>( f ), std::forward<Args>( args )... );
#else
		up_cntner_ = std::unique_ptr<cur_container_t>( new cur_container_t( std::forward<F>( f ), std::forward<Args>( args )... ) );
#endif
		p_cntner_             = up_cntner_.get();
	}

	template <typename F,
	          typename... Args,
	          typename std::enable_if<!std::is_same<F, deferred_apply>::value && ( buff_size >= sizeof( deferred_apply_container<R, F, Args...> ) )>::type* = nullptr>
	deferred_apply( F&& f, Args&&... args )
	  : up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		using cur_container_t = deferred_apply_container<R, F, Args&&...>;

		p_cntner_ = new ( placement_new_buffer ) cur_container_t( std::forward<F>( f ), std::forward<Args>( args )... );
	}
#endif   // __cpp_if_constexpr

	~deferred_apply()
	{
		if ( up_cntner_ != nullptr ) return;
		if ( p_cntner_ == nullptr ) return;

		p_cntner_->~deferred_apply_base();
	}

	R apply( void )
	{
		return p_cntner_->apply_func();
	}

private:
	std::unique_ptr<deferred_apply_base<R>> up_cntner_;
	deferred_apply_base<R>*                 p_cntner_;
	char                                    placement_new_buffer[buff_size];
};

/**
 * @brief 関数の実行を延期するために、関数と引数を保持することを目的としたクラスのインスタンスを生成するヘルパ関数
 *
 * @return deferred_apply<R>のインスタンス。Rは、 std::invoke_result<F, Args&&...>::type 。
 *
 */
template <typename F, typename... Args>
auto make_deferred_apply( F&& f, Args&&... args )
#if __cplusplus >= 201703L
	-> deferred_apply<typename std::invoke_result<F, Args&&...>::type>
#else
	-> deferred_apply<typename std::result_of<F( Args&&... )>::type>
#endif
{
#if __cplusplus >= 201703L
	using return_type = typename std::invoke_result<F, Args&&...>::type;
#else
	using return_type = typename std::result_of<F( Args && ... )>::type;
#endif
	return deferred_apply<return_type>( std::forward<F>( f ), std::forward<Args>( args )... );
}

/**
 * @brief 関数の実行を延期するために、関数と引数を保持することを目的としたクラスのインスタンスを生成するヘルパ関数
 *
 * Rは、 std::invoke_result<F, Args&&...>::type との間で変換可能であること。
 *
 * @return deferred_apply<R>のインスタンス。
 *
 */
template <typename R, typename F, typename... Args>
deferred_apply<R> make_deferred_apply_r( F&& f, Args&&... args )
{
	return deferred_apply<R>( std::forward<F>( f ), std::forward<Args>( args )... );
}

#endif
