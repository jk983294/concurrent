file(GLOB tool_tests "*.cpp")

enable_testing()

add_executable(test.tool ${tool_tests})
target_link_libraries(test.tool common_3rd_lib)
target_link_libraries(test.tool ${Boost_LIBRARIES} pthread rt)
