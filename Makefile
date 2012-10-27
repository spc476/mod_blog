
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

SHELL   = /bin/sh
SETUID  = chmod 4755

CC      = gcc -std=c99
CFLAGS  = -g -Wall -Wextra -pedantic
LFLAGS  =-rdynamic -lgdbm -lcgi6 `curl-config --libs` -llua -lm

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

all: build/boston

build/boston : build/addutil.o	\
		build/authenticate.o	\
		build/blog.o		\
		build/cgi_main.o		\
		build/cli_main.o		\
		build/globals.o		\
		build/main.o		\
		build/timeutil.o		\
		build/conversion.o		\
		build/callbacks.o		\
		build/wbtum.o		\
		build/backend.o		\
		build/blogutil.o		\
		build/entity-conversion.o	\
		build/facebook.o
	$(CC) $(CFLAGS) -o $@			\
		build/addutil.o		\
		build/authenticate.o	\
		build/blog.o		\
		build/cgi_main.o		\
		build/cli_main.o		\
		build/globals.o		\
		build/main.o		\
		build/timeutil.o		\
		build/conversion.o		\
		build/callbacks.o		\
		build/wbtum.o		\
		build/backend.o		\
		build/blogutil.o		\
		build/facebook.o		\
		build/entity-conversion.o	\
		$(LFLAGS) 
	$(SETUID) build/boston

#######################################################################
#
# Individual files
#
########################################################################

build/main.o : src/main.c		\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/main.c

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
	$(CC) $(CFLAGS) -c -o $@ src/backend.c

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
	$(CC) $(CFLAGS) -c -o $@ src/cli_main.c

build/cgi_main.o : src/cgi_main.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/cgi_main.c

build/authenticate.o : src/authenticate.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/authenticate.c

build/addutil.o : src/addutil.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/conversion.h	\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/addutil.c

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
	$(CC) $(CFLAGS) -c -o $@ src/callbacks.c

build/globals.o : src/globals.c	\
		src/backend.h		\
		src/blog.h		\
		src/conf.h		\
		src/conversion.h	\
		src/fix.h		\
		src/frontend.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

build/blog.o : src/blog.c		\
		src/backend.h		\
		src/blog.h		\
		src/conf.h 		\
		src/fix.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/blog.c

build/conversion.o : src/conversion.c	\
		src/blog.h		\
		src/conversion.h	\
		src/fix.h		\
		src/frontend.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/conversion.c

build/timeutil.o : src/timeutil.c src/timeutil.h
	$(CC) $(CFLAGS) -c -o $@ src/timeutil.c

build/wbtum.o : src/wbtum.c src/wbtum.h src/conf.h
	$(CC) $(CFLAGS) -c -o $@ src/wbtum.c

build/blogutil.o : src/blogutil.c src/blogutil.h
	$(CC) $(CFLAGS) -c -o $@ src/blogutil.c

build/entity-conversion.o : src/entity-conversion.c
	$(CC) $(CFLAGS) -c -o $@ src/entity-conversion.c

build/facebook.o : src/facebook.c	\
		src/backend.h		\
		src/blog.h		\
		src/frontend.h		\
		src/globals.h		\
		src/timeutil.h		\
		src/wbtum.h
	$(CC) $(CFLAGS) `curl-config --cflags` -c -o $@ src/facebook.c

#######################################################################
#
# Misc targets
#
#######################################################################

clean :
	/bin/rm -rf build/*.o
	/bin/rm -rf *~ src/*~

tarball:
	(cd .. ; tar czvf /tmp/boston.tar.gz -X boston/.exclude boston/ )

