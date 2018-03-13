cmake_minimum_required(VERSION 3.5)

project(concurrent)

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -Wall ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall ")
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

# Source code
include_directories(src)
include_directories("/home/$USER/include")

add_subdirectory(src)
add_subdirectory(example)
