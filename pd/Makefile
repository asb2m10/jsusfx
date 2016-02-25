PORTABLE=true
UNAME_S := $(shell uname -s)

ALL_CFLAGS = -I"$(PD_INCLUDE)" -DWDL_FFT_REALSIZE=8 -DEEL_TARGET_PORTABLE
ALL_LDFLAGS =

ifeq ($(UNAME_S),Darwin)
	## OS X
	EXTENSION=pd_darwin
	SHARED_EXTENSION = dylib
    OS = macosx
    PD_PATH = /Applications/Pd-extended.app/Contents/Resources
    OPT_CFLAGS = -ftree-vectorize -fast
    FAT_FLAGS = -arch i386 -arch x86_64 -mmacosx-version-min=10.5
    ALL_CFLAGS += $(FAT_FLAGS) -fPIC
    BUNDLE_LOADER = $(shell test ! -e $(PD_PATH)/bin/pd || echo -bundle_loader $(PD_PATH)/bin/pd)
    ALL_LDFLAGS += $(FAT_FLAGS) -headerpad_max_install_names -bundle $(BUNDLE_LOADER) -undefined dynamic_lookup
    SHARED_LDFLAGS += $(FAT_FLAGS) -dynamiclib -undefined dynamic_lookup -compatibility_version 1 -current_version 1.0
    STRIP = strip -x
else
	## Linux
	EXTENSION=pd_linux
  	ARCH := $(shell uname -m)
	CFLAGS=-O -g -DWDL_FFT_REALSIZE=8 -DPD -fPIC 
	LFLAGS=-rdynamic -shared -fPIC -Wl,-rpath,"\$$ORIGIN",--enable-new-dtags -Wl,-soname,jsusfx~.pd_linux -shared
	CPU := $(shell uname -m)
	SOURCES += $(SOURCES_linux)
	SHARED_EXTENSION = so
	OS = linux
	PD_PATH = /usr
	OPT_CFLAGS = -O6 -funroll-loops -fomit-frame-pointer
	ALL_CFLAGS += -fPIC
	ALL_LDFLAGS += -rdynamic -shared -fPIC -Wl,-rpath,"\$$ORIGIN",--enable-new-dtags
	SHARED_LDFLAGS += -Wl,-soname,$(SHARED_LIB) -shared
	STRIP = strip --strip-unneeded -R .note -R .comment
endif

OBJS=../jsusfx.o ../WDL/eel2/nseel-compiler.o ../WDL/eel2/nseel-eval.o ../WDL/eel2/nseel-ram.o ../WDL/eel2/nseel-yylex.o ../WDL/eel2/nseel-cfunc.o  ../WDL/fft.o jsusfx_pd.o
OBJS2=
PD_INCLUDE = $(PD_PATH)/include/pd

#asm-nseel-x64.o: ../WDL/eel2/a2x64.php ../WDL/eel2/asm-nseel-x86-gcc.c
#	php ../WDL/eel2/a2x64.php elf64

# ifdef PORTABLE
#   CFLAGS +=  -DEEL_TARGET_PORTABLE
# else
#   ifeq ($(UNAME_S),Darwin)
# 	ifeq ($(ARCH),x86_64)
# 	  OBJS2 += asm-nseel-x64-macho.o
# 	endif
#   endif
#   ifeq ($(UNAME_S),Linux)
# 	ifeq ($(ARCH),x86_64)
# 	  OBJS2 += asm-nseel-x64.o
# 	endif
#   endif
# endif
# CXXFLAGS=$(CFLAGS)

default: jsusfx~.$(EXTENSION)

%.o : %.c
	$(CC) $(ALL_CFLAGS) -c -o $@ $^

%.o : %.cpp
	$(CXX) $(ALL_CFLAGS) -c -o $@ $^

jsusfx~.$(EXTENSION): jsusfx_pd.o $(OBJS)
	$(CXX) $(ALL_LDFLAGS) -o "jsusfx~.$(EXTENSION)" $(OBJS)

clean:
	rm -f jsusfx~.$(EXTENSION) $(OBJS)

test: jsusfx~.$(EXTENSION)
	pd jsusfx_test.pd

debug: jsusfx~.$(EXTENSION)
	ddd pd test.pd