
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

.PHONY:	all clean tarball

MB_CURL_VERSION = $(shell curl-config --version)
MB_CURL_CFLAGS  = $(shell curl-config --cflags)
MB_CURL_LDLIBS  = $(shell curl-config --libs)
VERSION         = $(shell git describe --tag)

CC      = gcc -std=c99
CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g
LDLIBS  = -lgdbm -lcgi6 $(MB_CURL_LDLIBS) -llua -lm
SETUID  = /bin/chmod

override CFLAGS  += -DMBCURL_VERSION='"$(MB_CURL_VERSION)"' -DPROG_VERSION='"$(VERSION)"' $(MB_CURL_CFLAGS)
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
	$(RM) -r build *~ src/*~ depend

tarball:
	(cd .. ; tar czvf /tmp/boston.tar.gz -X boston/.exclude boston/ )

depend:
	makedepend -pbuild/ -f- -I/usr/local/include -- $(CFLAGS) -- src/*.c >depend
	
include depend
