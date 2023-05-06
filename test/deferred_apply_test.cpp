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

class testA
{
public:
    testA(void)
    {
        printf("%p: Called testA default-constructor\n", this);
    };
    testA(const testA &orig)
    {
        printf("%p: Called testA copy-constructor\n", this);
    };
    testA(testA &&orig)
    {
        printf("%p: Called testA move-constructor\n", this);
    };
    ~testA()
    {
        printf("%p: Called testA destructor\n", this);
    }
    testA &operator=(const testA &orig)
    {
        printf("%p: Called testA copy-assingment\n", this);
        return *this;
    };
    testA &operator=(testA &&orig)
    {
        printf("%p: Called testA move-assingment\n", this);
        return *this;
    };

    const char *c_str(void) const
    {
        return "ThisIsTestA";
    }
};

template <class... Args>
void deferred_printf(Args &&...args)
{
    std::cout << "Top: deferred_printf" << std::endl;

    argument_storage x(std::forward<Args>(args)...);
    std::cout << "XXX: stored arguments" << std::endl;
    x.apply();

    std::cout << "End: deferred_printf" << std::endl;
    return;
}

int main(void)
{
    static_assert(is_callable_c_str<testA const &>::value);

    // int   test_int = 1;
    testA aa{};

    // auto x1 = argument_storage( "a\n" );
    // auto x2 = argument_storage( "b, %d, %s\n", test_int, "c" );
    // auto x3 = argument_storage( "d, %d, %s, %s\n", 1, "e", testA {} );
    // auto x4 = argument_storage( "f, %d, %s, %s\n", 1, "g", aa );
    // auto x5 = argument_storage( "h, %d, %s, %s, %s\n", 1, aa, testA {}, "i" );

    // x1.print();
    // x2.print();
    // x3.print();
    // x4.print();
    // x5.print();

    deferred_printf("h, %d, %s, %s, %s\n", 1, aa, testA{}, "i");

    return EXIT_SUCCESS;
}