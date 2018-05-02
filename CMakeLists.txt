cmake_minimum_required(VERSION 3.5)

project(concurrent)

# boost lib
find_package( Boost 1.58.0 COMPONENTS system thread program_options filesystem )
include_directories(${Boost_INCLUDE_DIRS})

# tbb lib
list(APPEND CMAKE_MODULE_PATH "/home/$USER/github/CommonScript/linux/dev/compile/FindTBB/")
find_package(TBB COMPONENTS tbbmalloc tbbmalloc_proxy)

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -Wall ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall ")
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

# Source code
include_directories(src)
include_directories("/home/$USER/include")

add_subdirectory(src)
add_subdirectory(example)
