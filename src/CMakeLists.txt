file(GLOB CommonSrcs "*.cpp" "*/*.cpp" "*/*/*.cpp")
add_library(frenzy STATIC ${CommonSrcs})

install(DIRECTORY ./ DESTINATION /opt/version/latest/frenzy/include/
        FILES_MATCHING PATTERN "*.h")
install(TARGETS frenzy ARCHIVE DESTINATION /opt/version/latest/frenzy/lib/)