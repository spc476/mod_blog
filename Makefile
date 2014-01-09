
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

SHELL   = /bin/sh
SETUID  = chmod 4755

CC      = gcc -std=c99
CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g -rdynamic
LDLIBS  = -lgdbm -lcgi6 `curl-config --libs` -llua -lm

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

all: build build/boston

build/boston : build/addutil.o			\
		build/authenticate.o		\
		build/blog.o			\
		build/cgi_main.o		\
		build/cli_main.o		\
		build/globals.o			\
		build/main.o			\
		build/timeutil.o		\
		build/conversion.o		\
		build/callbacks.o		\
		build/wbtum.o			\
		build/backend.o			\
		build/blogutil.o		\
		build/entity-conversion.o	\
		build/facebook.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(SETUID) build/boston

#######################################################################
#
# Individual files
#
########################################################################

build/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o : src/main.c		\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h

build/backend.o : src/backend.c	\
		src/backend.h		\
		src/blog.h		\
		src/blogutil.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h

build/cli_main.o : src/cli_main.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h		\
		version.h

build/cgi_main.o : src/cgi_main.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h

build/authenticate.o : src/authenticate.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h		\
		version.h

build/addutil.o : src/addutil.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/conversion.h	\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h		\
		version.h

build/callbacks.o : src/callbacks.c	\
		src/backend.h		\
		src/blog.h		\
		src/blogutil.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h		\
		version.h

build/globals.o : src/globals.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/conversion.h	\
		src/fix.h		\
		src/frontend.h		\
		src/timeutil.h		\
		src/wbtum.h

build/blog.o : src/blog.c		\
		src/backend.h		\
		src/blog.h		\
		src/conf.h 		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h

build/conversion.o : src/conversion.c	\
		src/blog.h		\
		src/conversion.h	\
		src/fix.h		\
		src/frontend.h		\
		src/timeutil.h		\
		src/wbtum.h

build/timeutil.o : src/timeutil.c src/timeutil.h

build/wbtum.o : src/wbtum.c src/wbtum.h src/conf.h

build/blogutil.o : src/blogutil.c src/blogutil.h

build/entity-conversion.o : src/entity-conversion.c

build/facebook.o : src/facebook.c	\
		src/backend.h		\
		src/blog.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) `curl-config --cflags` -c -o $@ $<

version.h :
	scripts/post-commit

#######################################################################
#
# Misc targets
#
#######################################################################

build :
	mkdir -p build

clean :
	/bin/rm -rf build/*
	/bin/rm -rf *~ src/*~

tarball:
	(cd .. ; tar czvf /tmp/boston.tar.gz -X boston/.exclude boston/ )

