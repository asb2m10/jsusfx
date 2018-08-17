cmake_minimum_required(VERSION 2.6)

project(jsusfx)

# -- JSUSFX library --
set(sources
	jsusfx.cpp
	jsusfx_file.cpp
	jsusfx_gfx.cpp
	jsusfx_serialize.cpp
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
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm") 
		add_definitions(-mfpu=vfp -march=armv6t2 -marm)
		add_library(jsusfx ${sources} ${headers})
	else() 
		# Linux x64
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			add_custom_command(
				OUTPUT ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64.o
				COMMAND php a2x64.php elf64
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/WDL/eel2
			)
			add_library(jsusfx ${sources} ${headers} ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64.o)
		else()
			# Linux i686
			add_library(jsusfx ${sources} ${headers})
		endif()
	endif()
else()
	if (WIN32)
		add_library(jsusfx ${sources} ${headers})
	else()
		# MacOS, 64-bit always
		add_custom_command(
			OUTPUT ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64-macho.o
			COMMAND php a2x64.php macho64x
			DEPENDS ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x86-gcc.c
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/WDL/eel2
		)
		add_library(jsusfx ${sources} ${headers} ${PROJECT_SOURCE_DIR}/WDL/eel2/asm-nseel-x64-macho.o)
	endif()
endif()

target_compile_definitions(jsusfx PUBLIC WDL_FFT_REALSIZE=8)
target_include_directories(jsusfx PUBLIC "${PROJECT_SOURCE_DIR}")
