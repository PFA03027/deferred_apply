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

#include <cxxabi.h> // for abi::__cxa_demangle

#include <cstdlib>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

/* デバッグ用デマングル関数 */
inline char *demangle(const char *demangled)
{
    int status;
    return abi::__cxa_demangle(demangled, 0, 0, &status);
}

#if __cplusplus >= 201402L
// C++14以上の場合は、標準のインデックスシーケンスを使用する
template <size_t N>
using my_make_index_sequence = std::make_index_sequence<N>;
template <size_t... Is>
using my_index_sequence = std::index_sequence<Is...>;

#else

template <std::size_t...>
struct my_index_sequence
{
};

namespace internal
{
    template <std::size_t I, std::size_t... Is>
    struct S : S<I - 1, I - 1, Is...>
    {
    };

    template <std::size_t... Is>
    struct S<0, Is...>
    {
        my_index_sequence<Is...> f();
    };
} // namespace internal

template <std::size_t N>
using my_make_index_sequence = decltype(internal::S<N>{}.f());

#endif

/**
 * @brief const char*を返すメンバ関数c_str()を呼び出せるかどうかを検査するメタ関数の実装
 */
struct is_callable_c_str_impl
{
    template <typename T>
    static auto check(T *x) -> decltype(std::declval<T>().c_str(), std::true_type());

    template <typename T>
    static auto check(...) -> std::false_type;
};

/**
 * @brief const char*を返すメンバ関数c_str()を呼び出せるかどうかを検査するメタ関数
 */
template <typename T>
struct is_callable_c_str : decltype(is_callable_c_str_impl::check<typename std::remove_reference<T>::type>(nullptr))
{
};

/**
 * @brief 関数の実行を延期するために、一時的引数を保持することを目的としたクラス
 *
 * 一時的な保持を目的としているため、左辺値参照されたクラスは参照を保持する方式をとることで、コピーしない。
 * 代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。
 * よって、本クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。
 *
 */
struct deferred_apply
{
    constexpr static size_t buff_size = 256;

    ////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * @brief 引数を保持するためのtuple用の型を求めるメタ関数の実装クラス
     *
     * Tがunion or クラスの場合で左辺値参照ならば、型Tのconst参照型を返す
     * Tが配列型ならば、型Tの要素型のconstへのポインタ型を返す（decay+add_constを適用する）
     * Tが上記以外なら（＝組込み型、enum型や、右辺値参照のクラス等）なら、型Tをそのまま返す。
     */
    struct get_argument_store_type_impl
    {
        // 左辺値参照で、かつクラスかunion型の場合に、const左辺値参照を型として返す。
        template <typename T>
        static auto check(T x) -> typename std::enable_if<
            std::is_lvalue_reference<T>::value &&
                (std::is_class<typename std::remove_reference<T>::type>::value || std::is_union<typename std::remove_reference<T>::type>::value),
            typename std::add_lvalue_reference<typename std::add_const<typename std::remove_reference<T>::type>::type>::type>::type;

        // 配列型は、関数テンプレートと同じ推測を適用してconstへのポインタ型変換して返す。
        template <typename T>
        static auto check(T x) -> typename std::enable_if<
            std::is_pointer<typename std::decay<T>::type>::value,
            typename std::add_const<typename std::decay<T>::type>::type>::type;

        // 上記以外は、そのまま型を返す
        template <typename T>
        static auto check(...) -> typename std::remove_reference<T>::type;
    };

    /**
     * @brief 引数を保持するためのtuple用の型を求めるメタ関数
     */
    template <typename T>
    struct get_argument_store_type
    {
        using type = decltype(get_argument_store_type_impl::check<T>(std::declval<T>()));
    };

    ////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * @brief タプルの要素を関数の引数に適用する際の型を求めるメタ関数の実装クラス
     */
    struct get_argument_apply_type_impl
    {
        // 左辺値参照で、かつクラスかunion型の場合に、const左辺値参照を型として返す。
        template <typename T>
        static auto check(T x) -> typename std::enable_if<is_callable_c_str<T>::value, decltype(x.c_str())>::type;

        // 上記以外は、そのまま型を返す
        template <typename T>
        static auto check(...) -> typename std::remove_reference<T>::type;
    };

    /**
     * @brief タプルの要素を関数の引数に適用する際の型を求めるメタ関数
     */
    template <typename T>
    struct get_argument_apply_type
    {
        using type = decltype(get_argument_apply_type_impl::check<T>(std::declval<T>()));
    };

    template <size_t I, typename Tuple, typename std::enable_if<is_callable_c_str<typename std::tuple_element<I, typename std::remove_reference<Tuple>::type>::type>::value>::type * = nullptr>
    static auto get_argument_apply_value(Tuple &t) -> decltype(std::get<I>(t).c_str()) // C++11でもコンパイルできるようにする。C++14なら、戻り値推論+decltype(auto)を使うのが良い
    {
        return std::get<I>(t).c_str();
    }

    template <size_t I, typename Tuple, typename std::enable_if<!is_callable_c_str<typename std::tuple_element<I, typename std::remove_reference<Tuple>::type>::type>::value>::type * = nullptr>
    static auto get_argument_apply_value(Tuple &t) -> decltype(std::get<I>(t)) // C++11でもコンパイルできるようにする。C++14なら、戻り値推論+decltype(auto)を使うのが良い
    {
        return std::get<I>(t);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////
    class deferred_apply_carrier_base
    {
    public:
        virtual ~deferred_apply_carrier_base() = default;
        virtual void my_apply_func(void) = 0;
    };

    template <class... Args>
    struct deferred_apply_carrier : public deferred_apply_carrier_base
    {
        std::tuple<typename get_argument_store_type<Args>::type...> values_;

        template <class... XArgs>
        deferred_apply_carrier(XArgs &&...args)
            : values_(std::forward<XArgs>(args)...)
        {
            printf("values_: %s\n", demangle(typeid(values_).name()));
            printf("XArgs: %s\n", demangle(typeid(std::tuple<XArgs...>).name()));
        }

        void my_apply_func(void) override
        {
            my_apply_func_impl(my_make_index_sequence<sizeof...(Args)>());
            return;
        }

        template <size_t... Is>
        void my_apply_func_impl(my_index_sequence<Is...>)
        {
            printf("my_index_sequence: %s\n", demangle(typeid(my_index_sequence<Is...>).name()));

            // 保持している引数に対して、一定の処理を施し、その結果を適用したい関数に当てはめる。
            // ポイントは、「適用したい関数に当てはめる」ことだけでなく、「保持している引数に対して、一定の処理を施し」も遅延させていること。
            // ただ、「一定の処理」をテンプレート化する等の汎用化する方法がうまく浮かばない。。。
            using cur_apply_tuple_t = std::tuple<typename get_argument_apply_type<typename get_argument_store_type<Args>::type>::type...>;
            cur_apply_tuple_t apply_values(get_argument_apply_value<Is>(values_)...);
            printf("apply_values: %s\n", demangle(typeid(cur_apply_tuple_t).name()));
            printf(std::get<Is>(apply_values)...);
        }
    };

public:
    deferred_apply(const deferred_apply &orig) = delete;
    deferred_apply(deferred_apply &&orig) = delete;

#if __cpp_if_constexpr >= 201606
    template <class... Args>
    deferred_apply(Args &&...args)
        : up_args_(nullptr), p_args_(nullptr)
    {
        using cur_deferred_apply_t = deferred_apply_carrier<Args...>;
        printf("cur_deferred_apply_t: %s, size=%zu\n", demangle(typeid(cur_deferred_apply_t).name()), sizeof(cur_deferred_apply_t));

        if constexpr (buff_size < sizeof(cur_deferred_apply_t))
        { // 本来は、C++17から導入されたif constexpr構文を使用するのがあるべき姿
#if __cpp_lib_make_unique >= 201304
            up_args_ = std::make_unique<cur_deferred_apply_t>(std::forward<Args>(args)...);
#else
            up_args_ = std::unique_ptr<cur_deferred_apply_t>(new cur_deferred_apply_t(std::forward<Args>(args)...));
#endif
            p_args_ = up_args_.get();
        }
        else
        {
            p_args_ = new (placement_new_buffer) cur_deferred_apply_t(std::forward<Args>(args)...);
        }
    }
#else
    template <class... Args, typename std::enable_if<buff_size<sizeof(deferred_apply_carrier<Args...>)>::type * = nullptr> deferred_apply(Args &&...args) : up_args_(nullptr), p_args_(nullptr)
    {
        using cur_deferred_apply_t = deferred_apply_carrier<Args...>;
        printf("#1:cur_deferred_apply_t: %s, size=%zu\n", demangle(typeid(cur_deferred_apply_t).name()), sizeof(cur_deferred_apply_t));

#if __cpp_lib_make_unique >= 201304
        up_args_ = std::make_unique<cur_deferred_apply_t>(std::forward<Args>(args)...);
#else
        up_args_ = std::unique_ptr<cur_deferred_apply_t>(new cur_deferred_apply_t(std::forward<Args>(args)...));
#endif
        p_args_ = up_args_.get();
    }
    template <class... Args, typename std::enable_if<buff_size >= sizeof(deferred_apply_carrier<Args...>)>::type * = nullptr>
    deferred_apply(Args &&...args)
        : up_args_(nullptr), p_args_(nullptr)
    {
        using cur_deferred_apply_t = deferred_apply_carrier<Args...>;
        printf("#2:cur_deferred_apply_t: %s, size=%zu\n", demangle(typeid(cur_deferred_apply_t).name()), sizeof(cur_deferred_apply_t));

        p_args_ = new (placement_new_buffer) cur_deferred_apply_t(std::forward<Args>(args)...);
    }
#endif

    ~deferred_apply()
    {
    }

    void apply(void)
    {
        p_args_->my_apply_func();
        return;
    }

private:
    std::unique_ptr<deferred_apply_carrier_base> up_args_;
    deferred_apply_carrier_base *p_args_;
    char placement_new_buffer[buff_size];
};

#endif
