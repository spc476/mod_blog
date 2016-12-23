
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

.PHONY:	all clean tarball depend install uninstall reinstall

VERSION := $(shell git describe --tag)

CC      = c99 -Wall -Wextra -pedantic
CFLAGS  = -g
LDFLAGS = -g
LDLIBS  = -lgdbm -lcgi6 -llua -lm -ldl
SETUID  = /bin/chmod

INSTALL         = /usr/bin/install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_NAME    = boston

override CFLAGS  += -DPROG_VERSION='"$(VERSION)"' 
override LDFLAGS += -rdynamic

prefix = /usr/local
bindir = $(prefix)/bin

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

build/%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) build/boston $(DESTDIR)$(bindir)/$(INSTALL_NAME)
	$(SETUID) 4755 $(DESTDIR)$(bindir)/$(INSTALL_NAME)

uninstall:
	$(RM) $(DESTDIR)$(bindir)/$(INSTALL_NAME)

reinstall:
	make uninstall
	make install

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
	git archive -o /tmp/mod_blog-$(VERSION).tar.gz --prefix mod_blog $(VERSION)

depend:
	makedepend -pbuild/ -Y -- $(CFLAGS) -- src/*.c 2>/dev/null

# DO NOT DELETE

build/src/addutil.o: src/conf.h src/blog.h src/timeutil.h src/conversion.h
build/src/addutil.o: src/frontend.h src/wbtum.h src/globals.h src/backend.h
build/src/addutil.o: src/fix.h src/blogutil.h
build/src/authenticate.o: src/conf.h src/frontend.h src/wbtum.h
build/src/authenticate.o: src/timeutil.h src/blog.h src/globals.h
build/src/authenticate.o: src/backend.h
build/src/backend.o: src/fix.h src/frontend.h src/wbtum.h src/timeutil.h
build/src/backend.o: src/blog.h src/blogutil.h src/backend.h src/globals.h
build/src/blog.o: src/conf.h src/blog.h src/timeutil.h src/wbtum.h
build/src/blogutil.o: src/blogutil.h
build/src/callbacks.o: src/conf.h src/blog.h src/timeutil.h src/frontend.h
build/src/callbacks.o: src/wbtum.h src/fix.h src/blogutil.h src/globals.h
build/src/callbacks.o: src/backend.h
build/src/cgi_main.o: src/frontend.h src/wbtum.h src/timeutil.h src/blog.h
build/src/cgi_main.o: src/fix.h src/globals.h src/backend.h
build/src/cli_main.o: src/conf.h src/frontend.h src/wbtum.h src/timeutil.h
build/src/cli_main.o: src/blog.h src/fix.h src/blogutil.h src/globals.h
build/src/cli_main.o: src/backend.h
build/src/conversion.o: src/conversion.h src/fix.h src/frontend.h src/wbtum.h
build/src/conversion.o: src/timeutil.h src/blog.h src/blogutil.h
build/src/globals.o: src/conf.h src/conversion.h src/frontend.h src/wbtum.h
build/src/globals.o: src/timeutil.h src/blog.h src/backend.h src/fix.h
build/src/main.o: src/globals.h src/frontend.h src/wbtum.h src/timeutil.h
build/src/main.o: src/blog.h src/backend.h
build/src/timeutil.o: src/timeutil.h src/wbtum.h
build/src/wbtum.o: src/wbtum.h src/timeutil.h src/globals.h src/frontend.h
build/src/wbtum.o: src/blog.h src/backend.h
