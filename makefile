# Autoversion makefile
# (C) 2015-2016, Ruben Carlo Benante
# GNU License v2
#
# compilar
# g++ -Wall -Wextra -g -Og -xc++ -DVERSION="\"5.83.170416.015732\"" -DBUILD="\"170416.015732\"" xadreco.c -o xadreco

.PHONY: clean cleanall
.PRECIOUS: %.o
SHELL=/bin/bash -o pipefail

MAJOR = 10
MINOR = 0
BUILD = $(shell date +"%g%m%d.%H%M%S")
DEFSYM = $(subst .,_,$(BUILD))
VERSION = "\"$(MAJOR).$(MINOR).$(BUILD)\""
CC = g++
CFLAGS = -Wall -Wextra -g -Og -xc++
CPPFLAGS = -DVERSION=$(VERSION) -DBUILD="\"$(BUILD)\""
LDLIBS = -Wl,--defsym,BUILD_$(DEFSYM)=0
o = xadreco

#binary from source
$(o) : % : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDLIBS) $^ -o $@ |& tee errors.err
	echo $(o) version $(VERSION) > VERSION

clean:
	rm -f *.o errors.err

cleanall:
	rm -i $(o) *.x *.o errors.err

