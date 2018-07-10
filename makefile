# Autoversion makefile
# (C) 2015-2016, Ruben Carlo Benante
# GNU License v2
#
# Compilar para lichess
# 	make CONNECT=2 XDEBUG=1 DEBUG=1
#
# Compilar para fics
#   make CONNECT=1
#
# TODO:
#   random just for first move

.PHONY: clean cleanall
.PRECIOUS: %.o
SHELL=/bin/bash -o pipefail

MAJOR ?= 5
MINOR ?= 85
DEBUG ?= 0
BUILD = $(shell date +"%g%m%d.%H%M%S")
DEFSYM = $(subst .,_,$(BUILD))
VERSION = $(MAJOR).$(MINOR)
CC = gcc
CCW = i686-w64-mingw32-gcc 
CFLAGS = -Wall -Wextra -g -Og -c -std=gnu99 
# -Wno-unused-variable -Wno-unused-function
#CFLAGS = -Ofast -ansi -pedantic-errors
#RANDOM=-1:command line. RANDOM=-2:no random. RANDOM>=0: yes, seed N, 0=seed by time
RANDOM ?= -1
#CONNECT=0, 1 or 2: none, fics, lichess. CONNECT=3: command line decides
CONNECT ?= 3
#NOWAIT=1: no wait for xboard command. NOWAIT=0: command line decides
NOWAIT ?= 0
#XDEBUG==0: command line. XDEBUG>0, fixed value
XDEBUG ?= 0
CPPFLAGS = -DVERSION="\"$(VERSION)\"" -DBUILD="\"$(BUILD)\"" -DDEBUG=$(DEBUG) -DRANDOM=$(RANDOM) -DCONNECT=$(CONNECT) -DNOWAIT=$(NOWAIT) -DXDEBUG=$(XDEBUG)
LDLIBS = -Wl,--defsym,BUILD_$(DEFSYM)=0 -lm
#LDLIBS += -lgmp
#OBJ = libeco-ux64.o
LIBECO_BUILD = $(shell date +"%g%m%d.%H%M%S")
LIBECO_VERSION = 1.0
LIBECO_CFLAGS = -Ofast -c -Wno-unused-variable -Wno-unused-function
LIBECO_CPPFLAGS = -DDEBUG=0 -DVERSION=$(LIBECO_VERSION) -DBUILD=$(LIBECO_BUILD)
LIBECO_LDLIBS = -lm
o = xadreco

#object (Linux)
%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ |& tee errors.err

#binary ELF (Linux)
$(o) : % : %.o $(OBJ)
	$(CC) $(LDLIBS) $^ -o $@ |& tee errors.err
	echo $(o) version \"$(VERSION).$(BUILD)\" > VERSION

#binary EXE (Windows)
%.exe : %.obj
	$(CCW) $(LDLIBS) $(CPPFLAGS) $^ -o $@ |& tee errors.err

#object (Windows)
%.obj : %.c
	$(CCW) $(CFLAGS) $(CPPFLAGS) $^ -o $@ |& tee errors.err

#library
libeco-ux64.o : libeco.c
	$(CC) $(LIBECO_CFLAGS) $(LIBECO_CPPFLAGS) $(LDLIBS) libeco.c -o libeco-ux64.o

clean:
	rm -f *.o errors.err

cleanall:
	rm -i $(o) *.x *.o errors.err

