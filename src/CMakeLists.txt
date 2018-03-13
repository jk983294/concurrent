file(GLOB CommonSrcs "*.cpp" "*/*.cpp" "*/*/*.cpp")
add_library(concurrent_common_lib STATIC ${CommonSrcs})