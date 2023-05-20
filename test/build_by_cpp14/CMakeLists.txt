cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 14)	# for test purpose

file(GLOB SOURCES ../src/*.cpp )
# set(SOURCES test.cpp)

add_executable(test_deferred_apply_cpp14 ${SOURCES})
target_include_directories(test_deferred_apply_cpp14 PRIVATE ../../inc)
target_link_libraries(test_deferred_apply_cpp14 gtest gtest_main pthread)

add_test(NAME test_deferred_apply_cpp14 COMMAND $<TARGET_FILE:test_deferred_apply_cpp14>)
