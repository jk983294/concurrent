file(GLOB ExpSrcs "log/*.cpp")
add_library(frenzy_exp STATIC ${ExpSrcs})

add_subdirectory(example)