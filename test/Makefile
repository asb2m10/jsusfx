CC=gcc
CFLAGS=-O -g -DWDL_FFT_REALSIZE=8
LFLAGS=
CXX=g++
PORTABLE=true

OBJS=../jsusfx.o ../WDL/eel2/nseel-caltab.o ../WDL/eel2/nseel-compiler.o ../WDL/eel2/nseel-eval.o ../WDL/eel2/nseel-lextab.o ../WDL/eel2/nseel-ram.o ../WDL/eel2/nseel-yylex.o ../WDL/eel2/nseel-cfunc.o  ../WDL/fft.o jsusfx_test.o
OBJS2=

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  ifdef ARCH
    CFLAGS += -arch $(ARCH)
  else
    ARCH = i686
    CFLAGS += -arch i386
  endif
else
  ARCH := $(shell uname -m)
endif

default: jsusfx_test

%.o : %.mm
	$(CXX) $(CXXFLAGS) -c -o $@ $^

#asm-nseel-x64.o: ../WDL/eel2/a2x64.php ../WDL/eel2/asm-nseel-x86-gcc.c
#	php ../WDL/eel2/a2x64.php elf64

ifdef PORTABLE
  CFLAGS +=  -DEEL_TARGET_PORTABLE
else
  ifeq ($(UNAME_S),Darwin)
    ifeq ($(ARCH),x86_64)
      OBJS2 += asm-nseel-x64-macho.o
    endif
  endif
  ifeq ($(UNAME_S),Linux)
    ifeq ($(ARCH),x86_64)
      OBJS2 += asm-nseel-x64.o
    endif
  endif
endif
CXXFLAGS=$(CFLAGS)

jsusfx_test: jsusfx_test.o $(OBJS) $(OBJS2)
	g++ -o $@ $^ $(CXXFLAGS) $(LFLAGS)

test: jsusfx_test
	./jsusfx_test

clean:
	-rm jsusfx_test $(OBJS)
