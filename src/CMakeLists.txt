file(GLOB CommonSrcs "*.cpp" "*/*.cpp" "*/*/*.cpp")
add_library(frenzy STATIC ${CommonSrcs})

install(DIRECTORY ./ DESTINATION frenzy/include/ FILES_MATCHING PATTERN "*.h")
install(TARGETS frenzy ARCHIVE DESTINATION frenzy/lib/)