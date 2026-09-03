cmake_minimum_required(VERSION 3.16)
project(parallelcpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(PARALLELCPP_BUILD_TESTS "Build the Catch2 unit test suite" ON)
option(PARALLELCPP_BUILD_EXAMPLES "Build examples/" ON)

find_package(Threads REQUIRED)

add_library(parallelcpp STATIC src/thread_pool.cpp)
target_include_directories(parallelcpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(parallelcpp PUBLIC Threads::Threads)
target_compile_options(parallelcpp PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

if(PARALLELCPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(PARALLELCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
