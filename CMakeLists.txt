cmake_minimum_required(VERSION 3.5)

project(concurrent)

set(CMAKE_INSTALL_PREFIX "/opt/version/latest/")

# boost lib
find_package( Boost 1.58.0 COMPONENTS system thread program_options filesystem )
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

# tbb lib
#list(APPEND CMAKE_MODULE_PATH "/home/$USER/github/CommonScript/linux/dev/compile/FindTBB/")
#find_package(TBB COMPONENTS tbbmalloc tbbmalloc_proxy)

#SET(CXX_WARN_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast")
SET(CXX_WARN_FLAGS "-Wall -Wshadow -Wnon-virtual-dtor -Wcast-align -Woverloaded-virtual -Wpedantic -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g ${CXX_WARN_FLAGS} ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -Wall ")
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Source code
include_directories(src)
include_directories(exp)
include_directories(SYSTEM "/home/$USER/include")
include_directories(SYSTEM "/opt/3rd/common/include/")
include_directories(SYSTEM "zergtool/include")

link_directories(/opt/3rd/common/lib)

add_subdirectory(src)
add_subdirectory(zergtool)
#add_subdirectory(exp)
add_subdirectory(example)
add_subdirectory(test)

