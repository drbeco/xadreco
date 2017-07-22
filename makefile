# Autoversion makefile
# (C) 2015-2017, Ruben Carlo Benante
# GNU License v2

.PHONY: clean cleanall
.PRECIOUS: %.o
SHELL=/bin/bash -o pipefail

MAJOR = 10
MINOR = 1
BUILD = $(shell date +"%g%m%d.%H%M%S")
DEFSYM = $(subst .,_,$(BUILD))
VERSION = "\"$(MAJOR).$(MINOR).$(BUILD)\""
CC = gcc
CCW = i686-w64-mingw32-gcc
CFLAGS = -Wall -Wextra -g -Og -c -std=gnu99
CPPFLAGS = -DVERSION=$(VERSION) -DBUILD="\"$(BUILD)\"" -DDEEPXADRECO
LDLIBS = -Wl,--defsym,BUILD_$(DEFSYM)=0
o = xadreco

#binary from source
#$(o) : % : %.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDLIBS) $^ -o $@ |& tee errors.err
#	echo $(o) version $(VERSION) > VERSION
#

#object (Linux)
%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ |& tee errors.err

#binary ELF (Linux)
$(o) : % : %.o $(OBJ)
	$(CC) $(LDLIBS) $^ -o $@ |& tee errors.err
	echo $(o) version $(VERSION) > DEEPVERSION

#object (Windows)
%.obj : %.c
	$(CCW) $(CFLAGS) $(CPPFLAGS) $^ -o $@ |& tee errors.err

#binary EXE (Windows)
%.exe : %.obj
	$(CCW) $(LDLIBS) $(CPPFLAGS) $^ -o $@ |& tee errors.err
	echo $(o) version $(VERSION) > DEEPVERSION

clean:
	rm -f *.o errors.err

cleanall:
	rm -i $(o) *.x *.o errors.err

