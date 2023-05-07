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

template <class... Args>
void deferred_printf( Args&&... args )
{
	std::cout << "Top: deferred_printf" << std::endl;

	auto x = make_deferred_apply( printf, std::forward<Args>( args )... );
	std::cout << "XXX: stored arguments" << std::endl;
	x.apply();

	std::cout << "End: deferred_printf" << std::endl;
	return;
}

class deferred_format {
public:
	template <class... Args>
	deferred_format( const char* p_fmt_arg, Args&&... args )
	  : deferred_func_( snprintf, buff_, 2000 - 1, p_fmt_arg, std::forward<Args>( args )... )
	{
	}

	const char* c_str( void )
	{
		deferred_func_.apply();
		return buff_;
	}

private:
	deferred_apply<int> deferred_func_;
	char                buff_[2000];
};

void experiment_logger( bool output_flag, deferred_format&& df_str )
{
	if ( output_flag ) {
		printf( "%s", df_str.c_str() );
	} else {
		printf( "NO OUTPUT LOG\n" );
	}
}

int main( void )
{
	static_assert( is_callable_c_str<testA const&>::value );

	testA aa {};

	// deferred_printf( "a, %d, %s, %s, %s\n", 1, aa, testA {}, "b" );

	printf( "---------------------\n" );
	experiment_logger( true, deferred_format( "h, %d, %s, %s, %s\n", 1, aa, testA {}, "i" ) );
	// experiment_logger( true, deferred_format( "h, %d, %s\n", 1, "i" ) );
	printf( "---------------------\n" );
	// experiment_logger( false, deferred_format( "j, %d, %s, %s, %s\n", 1, aa, testA {}, "k" ) );
	experiment_logger( false, deferred_format( "j, %d, %s\n", 1, "k" ) );
	printf( "---------------------\n" );

	return EXIT_SUCCESS;
}