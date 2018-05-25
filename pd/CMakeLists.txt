cmake_minimum_required(VERSION 2.7)
project (jsusfx_pd)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

add_subdirectory(../src ../src)
include(pd.build/pd.cmake)
include_directories(../src)

set_pd_sources(/Applications/Pd.app/Contents/Resources/src/)
set_pd_external_path(.)

add_pd_external(jsusfx_pd_project jsusfx~ jsusfx_pd.cpp)

target_link_libraries(jsusfx_pd_project jsusfx)
