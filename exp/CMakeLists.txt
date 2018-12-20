file(GLOB ExpSrcs "*.cpp" "*/*.cpp" "*/*/*.cpp")
add_library(frenzy_exp STATIC ${ExpSrcs})