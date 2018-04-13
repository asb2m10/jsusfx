CFLAGS=-O -g -fPIC -DWDL_FFT_REALSIZE=8 
LFLAGS=-fPIC 
#PORTABLE=true
CXXFLAGS=-fPIC -DWDL_FFT_REALSIZE=8

OBJS=../jsusfx.o ../WDL/eel2/nseel-compiler.o ../WDL/eel2/nseel-eval.o ../WDL/eel2/nseel-ram.o ../WDL/eel2/nseel-yylex.o ../WDL/eel2/nseel-cfunc.o  ../WDL/fft.o jsusfx_test.o
OBJS2=

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	# by default, we take for granted that OS X builds are FAT (i386 + x86_64)
	OBJS2 += ../WDL/eel2/asm-nseel-x64-macho.o    
endif
ifeq ($(UNAME_S),Linux)
	ifeq ($(ARCH),x86_64)
	  OBJS2 += ../WDL/eel2/asm-nseel-x64.o
	endif
endif

default: jsusfx_test

# This is the assembly part that is only required for x64 ; php and nasm are required
../WDL/eel2/asm-nseel-x64-macho.o: 
	cd ../WDL/eel2 && php a2x64.php macho64x

../WDL/eel2/asm-nseel-x64.o: 
	cd ../WDL/eel2 && php a2x64.php elf64

OBJS2 += ../WDL/eel2/asm-nseel-x64.o

jsusfx_shared: 
	cc jsusfx_shared.c -o jsusfx_shared -ldl

jsusfx_test: jsusfx_test.o $(OBJS) $(OBJS2)
	g++ -o $@ $^ $(CXXFLAGS) $(LFLAGS)

jsusfx_test.so: jsusfx_test.o $(OBJS) $(OBJS2)
	g++ -shared -o $@ $^ $(CFLAGS) $(LFLAGS)

test: jsusfx_test
	./jsusfx_test

testso: jsusfx_shared jsusfx_test.so
	gdb ./jsusfx_shared

clean:
	rm -f jsusfx_test jsusfx_test.so jsusfx_shared
	rm $(OBJS) $(OBJS2)
