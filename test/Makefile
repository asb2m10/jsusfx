CFLAGS=-O -g -fPIC -DWDL_FFT_REALSIZE=8 
LFLAGS=-fPIC 
#PORTABLE=true
CXXFLAGS=-fPIC

OBJS=../jsusfx.o ../WDL/eel2/nseel-compiler.o ../WDL/eel2/nseel-eval.o ../WDL/eel2/nseel-ram.o ../WDL/eel2/nseel-yylex.o ../WDL/eel2/nseel-cfunc.o  ../WDL/fft.o jsusfx_test.o
OBJS2=

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  ifdef ARCH
    CFLAGS += -arch $(ARCH)
  else
    ARCH = i686
    CFLAGS += -arch i686
  endif
else
  ARCH := $(shell uname -m)
endif

default: jsusfx_test

#asm-nseel-x64.o: ../WDL/eel2/a2x64.php ../WDL/eel2/asm-nseel-x86-gcc.c
#	php ../WDL/eel2/a2x64.php elf64

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
