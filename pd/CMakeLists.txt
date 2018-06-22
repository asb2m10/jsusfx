cmake_minimum_required(VERSION 2.7)
project (jsusfx_pd)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
#set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

add_subdirectory(${PROJECT_SOURCE_DIR}/../src ${PROJECT_SOURCE_DIR}/../src)
include(pd.build/pd.cmake)
include_directories(${PROJECT_SOURCE_DIR}/../src)

if (WIN32)
	set_pd_sources(C:/Apps/pd/src)
else()
	set_pd_sources(/Applications/Pd.app/Contents/Resources/src/)
endif()

set_pd_external_path(${PROJECT_SOURCE_DIR})

add_pd_external(jsusfx_pd_project jsfx~ jsusfx_pd.cpp)

target_link_libraries(jsusfx_pd_project jsusfx)

# === Installation stuff ===
set(DIST ${PROJECT_SOURCE_DIR}/dist)

file(GLOB JSSCRIPT "${PROJECT_SOURCE_DIR}/../scripts/liteon/*.jsfx")

foreach(script ${JSSCRIPT})
	install(FILES ${script} DESTINATION ${DIST}/externals)
	install(CODE "execute_process(COMMAND python ${PROJECT_SOURCE_DIR}/jsfx2abstract.py ${script} WORKING_DIRECTORY ${DIST}/externals)" )
endforeach()

install(TARGETS jsusfx_pd_project DESTINATION ${DIST}/externals)
install(FILES ${PDHELP} DESTINATION ${DIST}/externals)
if (UNIX AND NOT APPLE)
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ./jsfx~.pd_darwin ${DIST}/externals/jsusfx~.pd_darwin)")
else()
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ./jsfx~.pd_linux ${DIST}/externals/jsusfx~.pd_linux)")
endif()

file(GLOB PDHELP "${PROJECT_SOURCE_DIR}/externals/*-help.pd")

install(FILES ${PROJECT_SOURCE_DIR}/jsfx2abstract.py DESTINATION ${DIST})
install(FILES ${PROJECT_SOURCE_DIR}/README.md DESTINATION ${DIST})
