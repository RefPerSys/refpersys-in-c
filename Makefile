
## Description:
##      This file is part of the Reflective Persistent System. refpersys.org
##
##      It is its Makefile, for the GNU make automation builder.
##
## Author(s):
##      Basile Starynkevitch <basile@starynkevitch.net>
##      Abhishek Chakravarti <abhishek@taranjali.org>
##      Nimesh Neema <nimeshneema@gmail.com>
 ##
##      © Copyright 2019 - 2021 The Reflective Persistent System Team
##      team@refpersys.org
##
## License:
##    This program is free software: you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation, either version 3 of the License, or
##    (at your option) any later version.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    You should have received a copy of the GNU General Public License
##    along with this program.  If not, see <http://www.gnu.org/lice

## tell GNU make to export all variables by default
export

RPS_GIT_ID:= $(shell tools/generate-gitid.sh)
RPS_SHORTGIT_ID:= $(shell tools/generate-gitid.sh -s)

RPS_PKG_CONFIG=  pkg-config
## libcurl is on curl.se/libcurl/ - a good HTTP client library
## jansson is on digip.org/jansson/ - a C library for JSON
## GTK3 is from gtk.org/ - a graphical interface library
RPS_PKG_NAMES= libcurl jansson gtk+-3.0
RPS_PKG_CFLAGS:= $(shell $(RPS_PKG_CONFIG) --cflags $(RPS_PKG_NAMES)) -I $(shell pwd)
RPS_PKG_LIBS:= $(shell $(RPS_PKG_CONFIG) --libs $(RPS_PKG_NAMES))

## handwritten C source files
RPS_C_SOURCES := $(sort $(wildcard src/[a-z]*_rps.c))
RPS_C_OBJECTS := $(patsubst %.c, %.o, $(RPS_C_SOURCES))
## unistring for UTF8 and Unicode: www.gnu.org/software/libunistring/
## libbacktrace for symbolic backtracing: github.com/ianlancetaylor/libbacktrace
## the libdl is POSIX dlopen and Linux dladdr
LDLIBES:= $(RPS_PKG_LIBS) -lunistring -lbacktrace -ldl
RM= rm -f
MV= mv

## the GNU indent program from www.gnu.org/software/indent/
INDENT= indent 

# Generated timestamp file prefix
RPS_TSTAMP:=generated/__timestamp

.PHONY: all clean objects indent redump load
.SECONDARY: $(RPS_TSTAMP).c

# the GCC compiler (at least GCC 9, preferably GCC 11, see gcc.gnu.org ....)
CC := gcc
## should be changed later to -Og, once loading succeeds
CFLAGS := -O0 -g3 -pg

## preprocessor flags for gcc
CPPFLAGS += $(RPS_PKG_CFLAGS) \
            -DRPS_GITID=\"$(RPS_GIT_ID)\" \
            -DRPS_SHORTGITID=\"$(RPS_SHORTGIT_ID)\" \
	    -I $(PWD)/include/
LDFLAGS += -rdynamic  -pie -Bdynamic -pthread -L /usr/local/lib -L /usr/lib

all:
	if [ -f refpersys ] ; then  $(MV) -f --backup refpersys refpersys~ ; fi
	$(RM) $(RPS_TSTAMP).c $(RPS_TSTAMP).o
	@echo "RPS_C_OBJECTS= " $(RPS_C_OBJECTS)
	sync
	$(MAKE) -$(MAKEFLAGS) refpersys
	sync

clean:
	$(RM) *.o */*.o */*.i *.orig *~ *% refpersys gmon.out

indent:
	for f in $(RPS_C_SOURCES) ; do \
	    $(INDENT) $$f ;  \
	done
	$(INDENT) Refpersys.h 

objects: $(RPS_C_OBJECTS)

# Target to generate timestamp source file
$(RPS_TSTAMP).c: | Makefile tools/generate-timestamp.sh
	tools/generate-timestamp.sh $@  > $@-tmp
	printf 'const char rps_c_compiler_version[]="%s";\n' "$$($(CC) --version | head -1)" >> $@-tmp
	printf 'const char _rps_git_short_id[] = "%s";\n' "$(RPS_SHORTGIT_ID)" >> $@-tmp
	$(MV) --backup $@-tmp $@


## preprocessed form
src/%_rps.i: src/%_rps.c
	$(CC) $(CPPFLAGS) -C -E $^ -o $@%
	sed '1,$$s:^#\(.*\)://\1:g' $@% > $@


## object files depend on common header file
src/%_rps.o: src/%_rps.c Refpersys.h kavl.h
	$(CC) $(CPPFLAGS) $^ -o $@

refpersys: objects $(RPS_TSTAMP).o
	$(LINK.c) $(LDFLAGS) $(RPS_C_OBJECTS) $(RPS_TSTAMP).o $(LDLIBES) -o $@
