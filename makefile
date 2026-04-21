# Autoversion makefile
# (C) 2015-2016, Ruben Carlo Benante
# GNU License v2
#
# Compilar para lichess
# 	make XDEBUG=1 DEBUG=1
#
# TODO:
#   random just for first move

.PHONY: clean cleanall deploy
.PRECIOUS: %.o
SHELL=/bin/bash -o pipefail

MAJOR ?= 7
MINOR ?= 10
DEBUG ?= 0
BUILD = $(shell date +"%g%m%d.%H%M%S")
DEFSYM = $(subst .,_,$(BUILD))
VERSION = $(MAJOR).$(MINOR)
CC = gcc
CFLAGS = -Wall -Wextra -g -Og -c -std=gnu99
# -Wno-unused-variable -Wno-unused-function
#CFLAGS = -Ofast -ansi -pedantic-errors
#RANDOM=-1:command line. RANDOM=-2:no random. RANDOM>=0: yes, seed N, 0=seed by time
RANDOM ?= -1
#NOWAIT=1: no wait for xboard command. NOWAIT=0: command line decides
NOWAIT ?= 0
#XDEBUG==0: command line. XDEBUG>0, fixed value
XDEBUG ?= 0
CPPFLAGS = -DVERSION="\"$(VERSION)\"" -DBUILD="\"$(BUILD)\"" -DDEBUG=$(DEBUG) -DRANDOM=$(RANDOM) -DNOWAIT=$(NOWAIT) -DXDEBUG=$(XDEBUG)
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

#library
libeco-ux64.o : libeco.c
	$(CC) $(LIBECO_CFLAGS) $(LIBECO_CPPFLAGS) $(LDLIBS) libeco.c -o libeco-ux64.o

deploy: $(o)
	cp $(o) $(o)-deploy
	$(eval DEPLOYVER := $(shell ./$(o)-deploy -V 2>&1 | sed -n 's/.*Version \([0-9]*\.[0-9]*\).*/\1/p' | head -1))
	sed -i 's/{me} v[0-9]*\.[0-9]*/{me} v$(DEPLOYVER)/' config.yml
	@echo "Deployed $(o)-deploy v$(DEPLOYVER)"

clean:
	rm -f *.o errors.err

cleanall:
	rm -i $(o) *.x *.o errors.err

