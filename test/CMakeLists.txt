SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../")

file(GLOB tests "*.cpp" "*/*.cpp")

enable_testing()

add_executable(all.test ${tests})
target_link_libraries(all.test concurrent_common_lib)
target_link_libraries(all.test ${Boost_LIBRARIES} tbb pthread rt)
