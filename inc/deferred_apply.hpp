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

#ifdef DEFERRED_APPLY_DEBUG
#include <cxxabi.h>   // for abi::__cxa_deferred_apply_internal::demangle
#endif

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
	// 配列型ではなく、かつ右辺値参照の場合、引数の値を保持する必要があるため、参照を外した型を返す。
	template <typename T>
	static auto check( T x ) -> typename std::enable_if<
		std::is_rvalue_reference<T>::value && !std::is_pointer<typename std::decay<T>::type>::value,
		typename std::remove_reference<T>::type>::type;

	// 配列型は、関数テンプレートと同じ推測を適用してた型に変換して返す。
	template <typename T>
	static auto check( T x ) -> typename std::enable_if<
		std::is_pointer<typename std::decay<T>::type>::value,
		typename std::decay<T>::type>::type;

	// 上記以外は、左辺値参照型となるため、そのまま型を返す。
	template <typename T>
	static auto check( ... ) -> T;
};

/**
 * @brief 引数を保持するためのtuple用の型を求めるメタ関数
 *
 * @li Tが配列型ではなく、かつ左辺値参照の場合に、左辺値参照を型として返す。
 * @li Tが配列型ならば、型Tの要素型のポインタ型を返す（decayを適用する）
 * @li Tが上記以外なら右辺値参照となる。引数を保持する必要があるため、参照を外した型を返す。
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
 * @brief Class intended to hold temporary arguments to defer execution of functions
 *
 * Example of use:
 * @code {.cpp}
 * auto da = make_deferred_applying_arguments( a, b, ... );
 * // do something, then...
 * auto ret = da.apply(f);
 * @endcode
 *
 * Execution of f(a,b,...) is delayed until da.apply(f).
 *
 * @warning
 * Whether reapplication of apply() is possible depends on how the arguments are passed or on the properties of f. @n
 * For example, if a is passed by rvalue reference, the value held in deferred_apply may be invalidated by f due to the first application of apply(). @n
 * Therefore, it is undefined whether the second apply() application is as intended.
 *
 * @warning
 * Since it is intended for temporary retention, priority is given to efficiency, and lvalue referenced instances are retained as lvalue reference types and are not copied. @n
 * Similarly, the array type is a method of holding a pointer to the array and not copying it. @n
 * Instead, a dangling reference can occur at the time the argument is actually referenced. @n
 * Therefore, instances of this class and their copies should not be brought out of the generated scope. @n
 * Rvalues and rvalue reference types are moved in order not to lose their values, and retain their values within this class. @n
 *
 * @note
 * You can do the same thing with a lambda capture, but
 * Since rvalues cannot be placed in the capture part, it is necessary to do it via a variable defined as an lvalue + reference capture.
 * Therefore, the implementation is quite troublesome.
 *
 * @brief 関数の実行を延期するために、一時的引数を保持することを目的としたクラス
 *
 * 使用例：
 * @code {.cpp}
 * auto da = make_deferred_applying_arguments( a, b, ... );
 * // do something, then...
 * auto ret = da.apply(f);
 * @endcode
 *
 * da.apply(f)まで、f(a,b,...) の実行が遅延される。
 *
 * @warning
 * apply()の再適用が可能かどうかは、引数をどのように渡したか、あるいはfの特性に依存する。 @n
 * 例えば、aが右辺値参照で引き渡された場合、1回目のapply()の適用によって、deferred_apply内で保持していた値が、fによって無効値となっている可能性がある。 @n
 * そのため、2回目のapply()適用が意図通りとなるかどうかは未定義となる。
 *
 * @warning
 * 一時的な保持を目的としているため、効率を優先し、左辺値参照されたインスタンスは左辺値参照型を保持する方式で、コピーしない。 @n
 * 配列型も同様に、配列へのポインタを保持する方式で、コピーしない。 @n
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。 @n
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。 @n
 * 右辺値や右辺値参照型は、値を失わないためにムーブし、本クラス内で値を保持する。 @n
 *
 * @note
 * ラムダ式のキャプチャを使うことでも同等のことが可能だが、
 * キャプチャ部分には右辺値を置けないため、左辺値として定義した変数＋参照キャプチャ経由で行う必要がある。
 * そのため、実装がかなり面倒。
 *
 */
template <typename... OrigArgs>
class deferred_applying_arguments {
public:
	deferred_applying_arguments( void )
	  : values_()
	{
	}
	deferred_applying_arguments( const deferred_applying_arguments& orig )
	  : values_( orig.values_ )
	{
	}
	deferred_applying_arguments( deferred_applying_arguments&& orig )
	  : values_( std::move( orig.values_ ) )
	{
	}
	deferred_applying_arguments& operator=( const deferred_applying_arguments& orig )
	{
		values_ = orig.values_;
		return *this;
	}
	deferred_applying_arguments& operator=( deferred_applying_arguments&& orig )
	{
		values_ = std::move( orig.values_ );
		return *this;
	}

	template <typename XArgsHead,
	          typename... XArgs,
	          typename std::enable_if<!std::is_same<typename std::remove_reference<XArgsHead>::type, deferred_applying_arguments>::value>::type* = nullptr>
	deferred_applying_arguments( XArgsHead&& argshead, XArgs&&... args )
	  : values_( std::forward<XArgsHead>( argshead ), std::forward<XArgs>( args )... )
	{
	}

	template <typename F>
#if __cpp_decltype_auto >= 201304
	decltype( auto ) apply( F&& f )
#else
	auto apply( F&& f ) -> typename std::result_of<F( OrigArgs... )>::type
#endif
	{
		return apply_impl( std::forward<F>( f ), deferred_apply_internal::my_make_index_sequence<std::tuple_size<tuple_args_t>::value>() );
	}

#ifdef DEFERRED_APPLY_DEBUG
	void debug_type_info( void )
	{
		printf( "Called constructor of deferred_applying_arguments\n" );
		printf( "\tthis class: %s\n", deferred_apply_internal::demangle( typeid( *this ).name() ) );
		printf( "\tvalues_: %s\n", deferred_apply_internal::demangle( typeid( values_ ).name() ) );
	}

	template <typename F>
	void debug_apply_type_info( F&& f )
	{
		printf( "f: %s\n", deferred_apply_internal::demangle( typeid( f ).name() ) );
		printf( "apply_impl: %s\n", deferred_apply_internal::demangle( typeid( decltype( apply_impl( std::forward<F>( f ), deferred_apply_internal::my_make_index_sequence<std::tuple_size<tuple_args_t>::value>() ) ) ).name() ) );
	}
#endif

private:
	template <typename F, size_t... Is>
#if __cpp_decltype_auto >= 201304
	decltype( auto ) apply_impl( F&& f, deferred_apply_internal::my_index_sequence<Is...> )
#else
	auto apply_impl( F&& f, deferred_apply_internal::my_index_sequence<Is...> ) -> typename std::result_of<F( OrigArgs... )>::type
#endif
	{
		return f( static_cast<
				  typename deferred_apply_internal::get_argument_apply_type<
					  OrigArgs,
					  typename deferred_apply_internal::get_argument_store_type<OrigArgs>::type>::type>(
			std::get<Is>( values_ ) )... );
	}

	using tuple_args_t = std::tuple<typename deferred_apply_internal::get_argument_store_type<OrigArgs>::type...>;
	tuple_args_t values_;
};

template <class... Args>
auto make_deferred_applying_arguments( Args&&... args ) -> decltype( deferred_applying_arguments<Args&&...>( std::forward<Args>( args )... ) )
{
	return deferred_applying_arguments<Args&&...>( std::forward<Args>( args )... );
}

namespace deferred_apply_internal {

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
	using funct_t                            = F;
	using argkeeper_t                        = deferred_applying_arguments<OrigArgs...>;
	static constexpr bool copy_constructible = std::is_copy_constructible<funct_t>::value && std::is_copy_constructible<argkeeper_t>::value;
	static constexpr bool move_constructible = std::is_move_constructible<funct_t>::value && std::is_move_constructible<argkeeper_t>::value;

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
		return placement_new_copy_impl( ptr );
	}
	deferred_apply_base<R>* placement_new_move( void* ptr ) override
	{
		return placement_new_move_impl( ptr );
	}
	std::unique_ptr<deferred_apply_base<R>> make_copy_clone( void ) override
	{
		return make_copy_clone_impl();
	}

#ifdef DEFERRED_APPLY_DEBUG
	void debug_type_info( void )
	{
		printf( "Called constructor of deferred_apply_container\n" );
		printf( "\tthis class: %s\n", deferred_apply_internal::demangle( typeid( *this ).name() ) );
		printf( "\targuments_keeper_: %s\n", deferred_apply_internal::demangle( typeid( arguments_keeper_ ).name() ) );
	}

#endif

	deferred_apply_container( const deferred_apply_container& orig )
	  : functor_( orig.functor_ )
	  , arguments_keeper_( orig.arguments_keeper_ )
	{
	}
	deferred_apply_container( deferred_apply_container&& orig )
	  : functor_( std::forward<F>( orig.functor_ ) )
	  , arguments_keeper_( std::move( orig.arguments_keeper_ ) )
	{
	}

private:
	template <bool IsCopyConstractable = copy_constructible, typename std::enable_if<IsCopyConstractable>::type* = nullptr>
	std::unique_ptr<deferred_apply_base<R>> make_copy_clone_impl( void )
	{
#if __cpp_lib_make_unique >= 201304
		return std::make_unique<deferred_apply_container>( *this );
#else
		return std::unique_ptr<deferred_apply_base<R>>( new deferred_apply_container( *this ) );
#endif
	}
	template <bool IsCopyConstractable = copy_constructible, typename std::enable_if<!IsCopyConstractable>::type* = nullptr>
	std::unique_ptr<deferred_apply_base<R>> make_copy_clone_impl( void )
	{
		throw( std::bad_alloc( "there is no copy constructor" ) );
	}

	template <bool IsCopyConstractable = copy_constructible, typename std::enable_if<IsCopyConstractable>::type* = nullptr>
	deferred_apply_base<R>* placement_new_copy_impl( void* ptr )
	{
		return new ( ptr ) deferred_apply_container( *this );
	}
	template <bool IsCopyConstractable = copy_constructible, typename std::enable_if<!IsCopyConstractable>::type* = nullptr>
	deferred_apply_base<R>* placement_new_copy_impl( void* ptr )
	{
		throw( std::bad_alloc() );
	}

	template <bool IsMoveConstractable = move_constructible, typename std::enable_if<IsMoveConstractable>::type* = nullptr>
	deferred_apply_base<R>* placement_new_move_impl( void* ptr )
	{
		return new ( ptr ) deferred_apply_container( std::move( *this ) );
	}
	template <bool IsMoveConstractable = move_constructible, typename std::enable_if<!IsMoveConstractable>::type* = nullptr>
	deferred_apply_base<R>* placement_new_move_impl( void* ptr )
	{
		throw( std::bad_alloc( "there is no move constructor" ) );
	}

	funct_t     functor_;
	argkeeper_t arguments_keeper_;
};

}   // namespace deferred_apply_internal

/**
 * @brief Class intended to hold temporary arguments to defer execution of functions
 *
 * Example of use:
 * @code {.cpp}
 * auto da = make_deferred_apply( f, a, b, ... );
 * // do something, then...
 * auto ret = da.apply();
 * @endcode
 *
 * @code {.cpp}
 * deferred_apply<R> da = make_deferred_apply_r<R>( f, a, b, ... );
 * // do something, then...
 * R ret = da.apply();
 * @endcode
 *
 * Execution of f(a,b,...) is delayed until da.apply().
 *
 * Unlike deferred_applying_arguments<>, only the return type for apply() is a template argument.
 * Therefore, it becomes easy to use it as a member variable of a class.
 * On the other hand, we cannot change the dynamically applied function f.
 *
 * @warning
 * Whether reapplication of apply() is possible depends on how the arguments are passed or on the properties of f. @n
 * For example, if a is passed by rvalue reference, the value held in deferred_apply may be invalidated by f due to the first application of apply(). @n
 * Therefore, it is undefined whether the second apply() application is as intended.
 *
 * @warning
 * Since it is intended for temporary retention, priority is given to efficiency, and lvalue referenced instances are retained as lvalue reference types and are not copied. @n
 * Similarly, the array type is a method of holding a pointer to the array and not copying it. @n
 * Instead, a dangling reference can occur at the time the argument is actually referenced. @n
 * Therefore, instances of this class and their copies should not be brought out of the generated scope. @n
 * Rvalues and rvalue reference types are moved in order not to lose their values, and retain their values within this class. @n
 *
 * @tparam R member function apply() return type
 *
 * @brief 関数の実行を延期するために、一時的引数を保持することを目的としたクラス
 *
 * 使用例：
 * @code {.cpp}
 * auto da = make_deferred_apply( f, a, b, ... );
 * // do something, then...
 * auto ret = da.apply();
 * @endcode
 *
 * @code {.cpp}
 * deferred_apply<R> da = make_deferred_apply_r<R>( f, a, b, ... );
 * // do something, then...
 * R ret = da.apply();
 * @endcode
 *
 * da.apply()まで、f(a,b,...) の実行が遅延される。
 *
 * deferred_applying_arguments<>とは異なり、apply()のための戻り値の型だけがテンプレート引数となる。
 * そのため、クラスのメンバ変数として使用することも容易になる。
 * 一方で、動的に適用する関数fを変更することはできない。
 *
 * @warning
 * apply()の再適用が可能かどうかは、引数をどのように渡したか、あるいはfの特性に依存する。 @n
 * 例えば、aが右辺値参照で引き渡された場合、1回目のapply()の適用によって、deferred_apply内で保持していた値が、fによって無効値となっている可能性がある。 @n
 * そのため、2回目のapply()適用が意図通りとなるかどうかは未定義となる。
 *
 * @warning
 * 一時的な保持を目的としているため、効率を優先し、左辺値参照されたインスタンスは左辺値参照型を保持する方式で、コピーしない。 @n
 * 配列型も同様に、配列へのポインタを保持する方式で、コピーしない。 @n
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。 @n
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。 @n
 * 右辺値や右辺値参照型は、値を失わないためにムーブし、本クラス内で値を保持する。 @n
 *
 * @tparam R メンバ関数apply()の戻り値の型
 */
template <typename R>
class deferred_apply {
	constexpr static size_t buff_size = 128;

public:
	deferred_apply( void )
	  : applying_count_( 0 )
	  , up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
	}
	deferred_apply( const deferred_apply& orig )
	  : applying_count_( orig.applying_count_ )
	  , up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		if ( orig.up_cntner_ != nullptr ) {
			up_cntner_ = orig.up_cntner_->make_copy_clone();
			p_cntner_  = up_cntner_.get();
		} else if ( orig.p_cntner_ != nullptr ) {
			p_cntner_ = orig.p_cntner_->placement_new_copy( placement_new_buffer );
		} else {
			// orig is empty object. Therefore, nothing to do
		}
	}
	deferred_apply( deferred_apply&& orig )
	  : applying_count_( orig.applying_count_ )
	  , up_cntner_( std::move( orig.up_cntner_ ) )
	  , p_cntner_( nullptr )
	{
		if ( up_cntner_ != nullptr ) {
			p_cntner_      = up_cntner_.get();
			orig.p_cntner_ = nullptr;
		} else if ( orig.p_cntner_ != nullptr ) {
			p_cntner_ = orig.p_cntner_->placement_new_move( placement_new_buffer );
			orig.p_cntner_->~deferred_apply_base();
			orig.p_cntner_ = nullptr;
		} else {
			// orig is empty object. Therefore, nothing to do
		}
		orig.applying_count_ = 0;
	}

#if __cpp_if_constexpr >= 201606
	template <typename F,
	          typename... Args,
	          typename std::enable_if<!std::is_same<typename std::remove_reference<F>::type, deferred_apply>::value>::type* = nullptr>
	deferred_apply( F&& f, Args&&... args )
	  : applying_count_( 0 )
	  , up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		using cur_container_t = deferred_apply_internal::deferred_apply_container<R, F, Args&&...>;

		if constexpr ( buff_size < sizeof( cur_container_t ) ) {   // C++17から導入されたif constexpr構文。C++11とC++14はSFINEで実装
			up_cntner_ = std::make_unique<cur_container_t>( std::forward<F>( f ), std::forward<Args>( args )... );
			p_cntner_  = up_cntner_.get();
		} else {
			p_cntner_ = new ( placement_new_buffer ) cur_container_t( std::forward<F>( f ), std::forward<Args>( args )... );
		}
	}
#else   // __cpp_if_constexpr
	template <typename F,
	          typename... Args,
	          typename std::enable_if<!std::is_same<typename std::remove_reference<F>::type, deferred_apply>::value && ( buff_size < sizeof( deferred_apply_internal::deferred_apply_container<R, typename std::remove_reference<F>::type, Args...> ) )>::type* = nullptr>
	deferred_apply( F&& f, Args&&... args )
	  : applying_count_( 0 )
	  , up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		using cur_container_t = deferred_apply_internal::deferred_apply_container<R, F, Args&&...>;

#if __cpp_lib_make_unique >= 201304
		up_cntner_            = std::make_unique<cur_container_t>( std::forward<F>( f ), std::forward<Args>( args )... );
#else
		up_cntner_ = std::unique_ptr<cur_container_t>( new cur_container_t( std::forward<F>( f ), std::forward<Args>( args )... ) );
#endif
		p_cntner_             = up_cntner_.get();
	}

	template <typename F,
	          typename... Args,
	          typename std::enable_if<( !std::is_same<typename std::remove_reference<F>::type, deferred_apply>::value ) && ( buff_size >= sizeof( deferred_apply_internal::deferred_apply_container<R, typename std::remove_reference<F>::type, Args...> ) )>::type* = nullptr>
	deferred_apply( F&& f, Args&&... args )
	  : applying_count_( 0 )
	  , up_cntner_( nullptr )
	  , p_cntner_( nullptr )
	{
		using cur_container_t = deferred_apply_internal::deferred_apply_container<R, F, Args&&...>;

		p_cntner_ = new ( placement_new_buffer ) cur_container_t( std::forward<F>( f ), std::forward<Args>( args )... );
	}
#endif   // __cpp_if_constexpr

	deferred_apply& operator=( const deferred_apply& orig )
	{
		// discard this
		if ( up_cntner_ != nullptr ) {
			up_cntner_.reset();
			p_cntner_ = nullptr;
		} else if ( p_cntner_ != nullptr ) {
			p_cntner_->~deferred_apply_base();
			p_cntner_ = nullptr;
		} else {
			// this object is empty object. Therefore, nothing to do
		}

		// copy to this
		if ( orig.up_cntner_ != nullptr ) {
			up_cntner_ = orig.up_cntner_->make_copy_clone();
			p_cntner_  = up_cntner_.get();
		} else if ( orig.p_cntner_ != nullptr ) {
			p_cntner_ = orig.p_cntner_->placement_new_copy( placement_new_buffer );
		} else {
			// orig is empty object. Therefore, nothing to do
		}
		applying_count_ = orig.applying_count_;

		return *this;
	}
	deferred_apply& operator=( deferred_apply&& orig )
	{
		// discard this
		if ( up_cntner_ != nullptr ) {
			up_cntner_.reset();
			p_cntner_ = nullptr;
		} else if ( p_cntner_ != nullptr ) {
			p_cntner_->~deferred_apply_base();
			p_cntner_ = nullptr;
		} else {
			// this object is empty object. Therefore, nothing to do
		}

		// move orig to this
		if ( orig.up_cntner_ != nullptr ) {
			up_cntner_     = std::move( orig.up_cntner_ );
			p_cntner_      = up_cntner_.get();
			orig.p_cntner_ = nullptr;
		} else if ( orig.p_cntner_ != nullptr ) {
			p_cntner_ = orig.p_cntner_->placement_new_move( placement_new_buffer );
			orig.p_cntner_->~deferred_apply_base();
			orig.p_cntner_ = nullptr;
		} else {
			// orig is empty object. Therefore, nothing to do
		}
		applying_count_      = orig.applying_count_;
		orig.applying_count_ = 0;

		return *this;
	}

	~deferred_apply()
	{
		if ( up_cntner_ != nullptr ) return;
		if ( p_cntner_ == nullptr ) return;

		p_cntner_->~deferred_apply_base();
	}

	R apply( void )
	{
		applying_count_++;
		return p_cntner_->apply_func();
	}

	int number_of_times_applied( void ) const
	{
		return applying_count_;
	}

	bool valid( void ) const
	{
		return ( p_cntner_ != nullptr );
	}

private:
	int                                                              applying_count_;
	std::unique_ptr<deferred_apply_internal::deferred_apply_base<R>> up_cntner_;
	deferred_apply_internal::deferred_apply_base<R>*                 p_cntner_;
	char                                                             placement_new_buffer[buff_size];
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
