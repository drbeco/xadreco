# Autoversion makefile
# (C) 2015-2016, Ruben Carlo Benante
# GNU License v2

.PHONY: clean cleanall
.PRECIOUS: %.o
SHELL=/bin/bash -o pipefail

MAJOR = 5
MINOR = 83
BUILD = $(shell date +"%g%m%d.%H%M%S")
DEFSYM = $(subst .,_,$(BUILD))
VERSION = "\"$(MAJOR).$(MINOR).$(BUILD)\""
CC = gcc
CCW = i686-w64-mingw32-gcc 
CFLAGS = -Wall -Wextra -g -Og -c -std=gnu99 
# -Wno-unused-variable -Wno-unused-function
#CFLAGS = -Ofast -ansi -pedantic-errors
CPPFLAGS = -DVERSION=$(VERSION) -DBUILD="\"$(BUILD)\""
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
	echo $(o) version $(VERSION) > VERSION

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

