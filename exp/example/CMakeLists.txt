file( GLOB APP_SOURCES "*.cpp" )
foreach( sourcefile ${APP_SOURCES} )
    # executable name cut off .cpp
    string( REGEX REPLACE ".+/" "" executablename1 ${sourcefile} )
    string( REPLACE ".cpp" "" executablename ${executablename1} )

    message ("source files: " ${sourcefile} " executable: " ${executablename})
    add_executable( ${executablename} ${sourcefile} )
    target_link_libraries( ${executablename} frenzy frenzy_exp pthread rt )
    target_link_libraries( ${executablename} ${Boost_LIBRARIES})
endforeach( sourcefile ${APP_SOURCES} )