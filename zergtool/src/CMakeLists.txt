file(GLOB ztoolSrc "*.cpp")
add_library(zergtool STATIC ${ztoolSrc})
set_property(TARGET zergtool PROPERTY POSITION_INDEPENDENT_CODE ON)
install(TARGETS zergtool ARCHIVE DESTINATION lib)
