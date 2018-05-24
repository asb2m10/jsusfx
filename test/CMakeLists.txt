cmake_minimum_required(VERSION 2.7)
project (jsusfx_test)

add_subdirectory(../src ../src)
set(SOURCE jsusfx_test.cpp)

add_executable(jsusfx_test jsusfx_test.cpp)
target_link_libraries(jsusfx_test jsusfx)
