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

VERSION := $(shell git describe --tag)

ifeq ($(VERSION),)
  VERSION=v45
endif

CC      = gcc -std=c99 -Wall -Wextra -pedantic -Wwrite-strings
CFLAGS  = -g
LDFLAGS = -g
LDLIBS  = -lgdbm -lcgi6 -llua -lm -ldl
SETUID  = /bin/chmod

INSTALL         = /usr/bin/install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_NAME    = boston

override CFLAGS  +=-D_GNU_SOURCE -DPROG_VERSION='"$(VERSION)"'
override LDFLAGS +=-rdynamic

prefix = /usr/local
bindir = $(prefix)/bin

#######################################################################

.PHONY: clean dist depend install uninstall reinstall

src/main : $(patsubst %.c,%.o,$(wildcard src/*.c))

install:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) src/main $(DESTDIR)$(bindir)/$(INSTALL_NAME)
	$(SETUID) 4755 $(DESTDIR)$(bindir)/$(INSTALL_NAME)

uninstall:
	$(RM) $(DESTDIR)$(bindir)/$(INSTALL_NAME)

reinstall:
	make uninstall
	make install

clean :
	$(RM) $(shell find . -name '*~')
	$(RM) $(shell find . -name '*.o')
	$(RM) src/main Makefile.bak

dist:
	git archive -o /tmp/mod_blog-$(VERSION).tar.gz --prefix mod_blog/ $(VERSION)

depend:
	makedepend -Y -- $(CFLAGS) -- src/*.c 2>/dev/null

# DO NOT DELETE

src/addutil.o: src/conversion.h src/frontend.h src/wbtum.h src/timeutil.h
src/addutil.o: src/blog.h src/backend.h src/blogutil.h
src/authenticate.o: src/frontend.h src/wbtum.h src/timeutil.h src/blog.h
src/backend.o: src/blogutil.h src/backend.h src/frontend.h src/wbtum.h
src/backend.o: src/timeutil.h src/blog.h
src/blog.o: src/blog.h src/timeutil.h src/wbtum.h
src/blogutil.o: src/blogutil.h
src/callbacks.o: src/backend.h src/frontend.h src/wbtum.h src/timeutil.h
src/callbacks.o: src/blog.h src/blogutil.h src/conversion.h
src/conversion.o: src/conversion.h src/frontend.h src/wbtum.h src/timeutil.h
src/conversion.o: src/blog.h src/blogutil.h
src/main.o: src/main.h src/frontend.h src/wbtum.h src/timeutil.h src/blog.h
src/main_cgi.o: src/backend.h src/frontend.h src/wbtum.h src/timeutil.h
src/main_cgi.o: src/blog.h src/conversion.h src/main.h
src/main_cli.o: src/backend.h src/frontend.h src/wbtum.h src/timeutil.h
src/main_cli.o: src/blog.h src/blogutil.h src/conversion.h src/main.h
src/timeutil.o: src/wbtum.h src/timeutil.h
src/wbtum.o: src/wbtum.h src/timeutil.h
