/**
 * @file deferred_apply_test.cpp
 * @author PFA03027@nifty.com
 * @brief 引数情報を保持するクラスのテスト
 * @version 0.1
 * @date 2023-05-05
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cstdio>
#include <cstdlib>
#include <memory>

#include "deferred_apply_test.hpp"

int main( void )
{
	int exit_code = 0;
	exit_code += test1();
	exit_code += test_printf_with_convert();

	return exit_code;
}
