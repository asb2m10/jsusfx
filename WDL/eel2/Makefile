CC=gcc
CFLAGS=-O -g -DWDL_FFT_REALSIZE=8
LFLAGS=
CXX=g++


OBJS=nseel-caltab.o nseel-compiler.o nseel-ram.o nseel-yylex.o nseel-cfunc.o  ../fft.o

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

nseel-compiler.o: glue*.h ns-eel*.h
nseel-cfunc.o:  asm*.c ns-eel*.h
nseel-*.o: ns-eel*.h

ifdef MAXLOOP
  CFLAGS += -DNSEEL_LOOPFUNC_SUPPORT_MAXLEN=$(MAXLOOP)
else
  CFLAGS += -DNSEEL_LOOPFUNC_SUPPORT_MAXLEN=0
endif


%.o : %.mm
	$(CXX) $(CXXFLAGS) -c -o $@ $^

asm-nseel-x64.o: a2x64.php asm-nseel-x86-gcc.c
	php a2x64.php elf64

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

loose_eel: loose_eel.o $(OBJS) $(OBJS2)
	g++ -o $@ $^ $(CXXFLAGS) $(LFLAGS)

clean:
	ls -l $(OBJS)
