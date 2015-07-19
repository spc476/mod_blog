
#########################################################################
#
# Copyright 2000 by Sean Conner.  All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# Comments, questions and criticisms can be sent to: sean@conman.org
#
########################################################################

.PHONY:	all clean tarball depend

VERSION         = $(shell git describe --tag)

CC      = gcc -std=c99
CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g
LDLIBS  = -lgdbm -lcgi6 -llua -lm -ldl
SETUID  = /bin/chmod

override CFLAGS  += -DPROG_VERSION='"$(VERSION)"' 
override LDFLAGS += -rdynamic

###################################
# some notes
#
# for GCC, profiling information can be had with:
#CFLAGS=-pg
#LFLAGS=-lcing2d
#
# and running gprof after running the program
#
################################################

all: build build/src build/boston

build/boston : $(addprefix build/,$(patsubst %.c,%.o,$(wildcard src/*.c)))
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(SETUID) 4755 build/boston

build/%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

#######################################################################
#
# Misc targets
#
#######################################################################

build build/src :
	mkdir -p $@

clean :
	$(RM) -r build *~ src/*~ *.bak

tarball:
	(cd .. ; tar czvf /tmp/boston.tar.gz -X boston/.exclude boston/ )

depend:
	makedepend -pbuild/ -Y -- $(CFLAGS) -- src/*.c 2>/dev/null

# DO NOT DELETE

build/src/addutil.o: src/conf.h src/blog.h src/timeutil.h src/conversion.h
build/src/addutil.o: src/frontend.h src/wbtum.h src/backend.h src/globals.h
build/src/addutil.o: src/fix.h
build/src/authenticate.o: src/conf.h src/timeutil.h src/frontend.h
build/src/authenticate.o: src/wbtum.h src/blog.h src/backend.h src/globals.h
build/src/backend.o: src/conf.h src/blog.h src/timeutil.h src/wbtum.h
build/src/backend.o: src/frontend.h src/fix.h src/blogutil.h src/backend.h
build/src/backend.o: src/globals.h
build/src/blog.o: src/conf.h src/blog.h src/timeutil.h src/frontend.h
build/src/blog.o: src/wbtum.h src/backend.h src/fix.h
build/src/blogutil.o: src/blogutil.h
build/src/callbacks.o: src/conf.h src/blog.h src/timeutil.h src/wbtum.h
build/src/callbacks.o: src/frontend.h src/backend.h src/fix.h src/blogutil.h
build/src/callbacks.o: src/globals.h
build/src/cgi_main.o: src/conf.h src/blog.h src/timeutil.h src/frontend.h
build/src/cgi_main.o: src/wbtum.h src/backend.h src/fix.h src/globals.h
build/src/cli_main.o: src/conf.h src/blog.h src/timeutil.h src/frontend.h
build/src/cli_main.o: src/wbtum.h src/fix.h src/backend.h src/globals.h
build/src/conversion.o: src/conversion.h src/fix.h src/frontend.h src/wbtum.h
build/src/conversion.o: src/timeutil.h src/blog.h
build/src/globals.o: src/conf.h src/conversion.h src/frontend.h src/wbtum.h
build/src/globals.o: src/timeutil.h src/blog.h src/backend.h src/fix.h
build/src/main.o: src/conf.h src/blog.h src/timeutil.h src/fix.h
build/src/main.o: src/frontend.h src/wbtum.h src/backend.h src/globals.h
build/src/timeutil.o: src/timeutil.h src/wbtum.h
build/src/wbtum.o: src/wbtum.h src/timeutil.h src/globals.h src/frontend.h
build/src/wbtum.o: src/blog.h src/backend.h
