find_package(Arrow REQUIRED)
file( GLOB EXAMPLE_TOOL_SOURCES "*.cpp" )
foreach( sourcefile ${EXAMPLE_TOOL_SOURCES} )
    # executable name cut off .cpp
    string( REGEX REPLACE ".+/" "example_tool_" executablename1 ${sourcefile} )
    string( REPLACE ".cpp" "" executablename ${executablename1} )

    message ("source files: " ${sourcefile} " executable: " ${executablename})
    add_executable( ${executablename} ${sourcefile} )
    target_link_libraries( ${executablename} common_3rd_lib frenzy ${OpenMP_CXX_FLAGS}
            ${FST_LIB} ${LZ4_LIB} ${ZSTD_LIB} arrow pthread rt dl)
endforeach( sourcefile ${EXAMPLE_TOOL_SOURCES} )
