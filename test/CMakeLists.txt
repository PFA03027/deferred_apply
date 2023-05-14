cmake_policy(SET CMP0048 NEW)
project(DeferredApplyTest, VERSION 0.1.0)
cmake_minimum_required(VERSION 3.16)

find_package(Git QUIET)

# Gitがインストールされており、トップディレクトリに.gitディレクトリがある
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/../.git")
    enable_testing()
    option(GOOGLETEST "Check Google Test during build" ON)
    # -DGOOGLETEST=onオプションを付けて実行したらsubmoduleを最新版にする
    if(GOOGLETEST)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/googletest/CMakeLists.txt")
message(FATAL_ERROR "The submodules were not downloaded! GOOGLETEST was turned off or failed. Please update submodules and try again.")
# git submodule add https://github.com/google/googletest.git googletest
# cd googletest
# git checkout release-1.12.1
# cd ..
# git add googletest
# git commit -m "Add submodule googletest for testing"
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DDEFERRED_APPLY_DEBUG")

add_subdirectory(googletest)
add_subdirectory(build_by_cpp11)
add_subdirectory(build_by_cpp14)
add_subdirectory(build_by_cpp17)

