cmake_minimum_required(VERSION 2.6)

project(jsusfx)

# -- JSUSFX library --
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
set(sources
	jsusfx.cpp
	jsusfx_file.cpp
	jsusfx_gfx.cpp
	riff.cpp
	WDL/eel2/nseel-compiler.c
	WDL/eel2/nseel-eval.c
	WDL/eel2/nseel-ram.c
	WDL/eel2/nseel-yylex.c
	WDL/eel2/nseel-cfunc.c
	WDL/fft.c)

file(GLOB_RECURSE headers *.h)

source_group("sources" FILES ${sources})
source_group("headers" FILES ${headers})

if (UNIX AND NOT APPLE)
	add_custom_command(
		OUTPUT ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64.o
		COMMAND php a2x64.php elf64
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/WDL/eel2
	)
	add_library(jsusfx ${sources} ${headers} ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x86.o)
else()
	add_custom_command(
		OUTPUT ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64-macho.o
		COMMAND php a2x64.php macho64x
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/WDL/eel2
	)
	add_library(jsusfx ${sources} ${headers} ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64-macho.o)
endif()

target_compile_definitions(jsusfx PUBLIC WDL_FFT_REALSIZE=8)
target_include_directories(jsusfx PUBLIC "${PROJECT_SOURCE_DIR}")

