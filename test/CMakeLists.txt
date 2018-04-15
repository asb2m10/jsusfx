cmake_minimum_required(VERSION 2.6)

project(jsusfx_test)

set(sources
	../jsusfx.cpp
	../jsusfx_gfx.cpp
	../WDL/eel2/nseel-compiler.c
	../WDL/eel2/nseel-eval.c
	../WDL/eel2/nseel-ram.c
	../WDL/eel2/nseel-yylex.c
	../WDL/eel2/nseel-cfunc.c
	../WDL/fft.c
	jsusfx_test.cpp)

file(GLOB_RECURSE headers ../*.h)

source_group("sources" FILES ${sources})
source_group("headers" FILES ${headers})

add_executable(jsusfx_test ${sources} ${headers} ../WDL/eel2/asm-nseel-x64-macho.o)

target_compile_definitions(jsusfx_test PRIVATE WDL_FFT_REALSIZE=8)

#target_link_libraries(jsusfx_test ../WDL/eel2/asm-nseel-x64-macho.o)