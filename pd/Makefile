CC=clang
CFLAGS=-O -g -DWDL_FFT_REALSIZE=8 -DPD -fPIC 
LFLAGS=-rdynamic -shared -fPIC -Wl,-rpath,"\$$ORIGIN",--enable-new-dtags -Wl,-soname,jsusfx~.pd_linux -shared
CXX=clang++
PORTABLE=true

OBJS=../jsusfx.o ../WDL/eel2/nseel-caltab.o ../WDL/eel2/nseel-compiler.o ../WDL/eel2/nseel-eval.o ../WDL/eel2/nseel-lextab.o ../WDL/eel2/nseel-ram.o ../WDL/eel2/nseel-yylex.o ../WDL/eel2/nseel-cfunc.o  ../WDL/fft.o jsusfx_pd.o
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

default: jsusfx~.pd_linux

jsusfx_pd.o: jsusfx_pd.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c -o $@ $^

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

jsusfx.pd_linux: jsusfx_pd.o $(OBJS) $(OBJS2)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LFLAGS)

jsusfx~.pd_linux: jsusfx.pd_linux
	mv jsusfx.pd_linux jsusfx~.pd_linux

clean:
	rm jsusfx~.pd_linux $(OBJS)

test: jsusfx~.pd_linux
	pd jsusfx_test.pd

debug: jsusfx~.pd_linux
	ddd pd test.pd