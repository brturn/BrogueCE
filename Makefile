include config.mk

# cflags := -std=c99
cflags := -Isrc/brogue -Isrc/platform -Isrc/variants \
	-Wall -Wpedantic -Werror=implicit -Wno-parentheses -Wno-unused-result \
	-Wformat -Werror=format-security -Wno-format-overflow -Wmissing-prototypes
libs := -lm
cppflags := -DDATADIR=$(DATADIR)

sources := $(wildcard src/brogue/*.c) $(wildcard src/variants/*.c) $(addprefix src/platform/,main.c platformdependent.c null-platform.c)
objects :=

ifeq ($(SYSTEM),WINDOWS)
objects += windows/resources.o
.exe := .exe
endif

ifeq ($(SYSTEM),OS2)
cflags += -march=pentium4 -mtune=pentium4 -Zmt -Wno-narrowing
cppflags += -D__ST_MT_ERRNO__
libs += -lcx -lc -Zomf -Zbin-files -Zargs-wild -Zargs-resp -Zhigh-mem -Zstack 8000
objects += os2/icon.res os2/brogue.lib
.exe := .exe
endif

ifeq ($(RELEASE),YES)
extra_version :=
else
extra_version := $(shell bash tools/git-extra-version)
endif

ifeq ($(TERMINAL),YES)
sources += $(addprefix src/platform/,curses-platform.c term.c)
cppflags += -DBROGUE_CURSES
libs += -lncurses
ifeq ($(SYSTEM),OS2)
libs += -ltinfo
endif
endif

ifeq ($(GRAPHICS),YES)
sources += $(addprefix src/platform/,sdl2-platform.c tiles.c)
cflags += $(shell $(SDL_CONFIG) --cflags)
cppflags += -DBROGUE_SDL
libs += $(shell $(SDL_CONFIG) --libs) -lSDL2_image
endif

ifeq ($(WEBBROGUE),YES)
sources += $(addprefix src/platform/,web-platform.c)
cppflags += -DBROGUE_WEB
endif

ifeq ($(JSBROGUE),YES)
sources += $(addprefix src/platform/,javascript-platform.c)
cppflags += -DBROGUE_JS
cflags += -std=gnu11 -Wbad-function-cast -Wcast-function-type
CC = emcc -sASYNCIFY 
#-pthread -sMAIN_MODULE=1

libs += -sASSERTIONS -sSAFE_HEAP -sEXCEPTION_DEBUG -sASYNCIFY 
#
# -sLIBRARY_DEBUG 
#-sALLOW_MEMORY_GROWTH=1 -sALLOW_TABLE_GROWTH -sFULL_ES3=1 -sFORCE_FILESYSTEM=1 
#-sEXCEPTION_DEBUG -sLIBRARY_DEBUG 
#-sSYSCALL_DEBUG -sDYLINK_DEBUG -sFS_DEBUG

# LIBRARIES = -sASYNCIFY -sASSERTIONS
.exe = .html

# -s SAFE_HEAP 
# -s ASSERTIONS=1 
# -s ALLOW_MEMORY_GROWTH=1 
# -s PTHREAD_POOL_SIZE=3 
# -s ASYNCIFY -pthread 
# -s ASYNCIFY_IMPORTS=['doLoadLibrary'] 
# -s ALLOW_TABLE_GROWTH
# -s USE_PTHREADS=1
# -s PROXY_TO_PTHREAD
# -s EXPORTED_RUNTIME_METHODS=['FS']
# -s DYNCALLS=1
# -s MAIN_MODULE=1
# -s USE_WEBGL2=1
# -s FULL_ES3=1
# -s OFFSCREEN_FRAMEBUFFER=1
# -s FORCE_FILESYSTEM=1
# --profiling
# --pre-js=
endif


ifeq ($(RAPIDBROGUE),YES)
cppflags += -DRAPID_BROGUE
endif

ifeq ($(MAC_APP),YES)
cppflags += -DSDL_PATHS
endif

ifeq ($(DEBUG),YES)
cflags += -g -Og
cppflags += -DENABLE_PLAYBACK_SWITCH
else
cflags += -O2
endif

# Add user-provided flags.
cflags += $(CFLAGS)
cppflags += $(CPPFLAGS)
libs += $(LDLIBS)

objects += $(sources:.c=.o)

include make/*.mk
.DEFAULT_GOAL := bin/brogue$(.exe)

clean:
	$(warning 'make clean' is no longer needed in many situations, so is not supported. Use 'make -B' to force rebuild something.)
	rm src/*/*.o

escape = $(subst ','\'',$(1))
vars:
	mkdir -p vars
# Write the value to a temporary file and only overwrite if it's different.
vars/%: vars FORCE
	@echo '$(call escape,$($*))' > vars/$*.tmp
	@if ! cmp --quiet vars/$*.tmp vars/$*; then cp vars/$*.tmp vars/$*; fi


FORCE:
