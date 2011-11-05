
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

SHELL=/bin/sh
HOSTDIR=build
SETUID=chmod 4755

CC=gcc -g -Wall -pedantic -std=c99 -Wextra
CINCL=

#CFLAGS=-g -Wall -pedantic -Wpointer-arith -Wshadow -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wcast-qual -Waggregate-return -Wmissing-declarations -Wnested-externs -Winline -W $(CINCL)
#CFLAGS=-DSCREAM -O4 -fomit-frame-pointer  $(CINCL)
#CFLAGS=-O3 -fomit-frame-pointer  -DSCREAM $(CINCL)
#CFLAGS=-O3 -fomit-frame-pointer  $(CINCL)
#CFLAGS=-pg -g -DSCREAM -DNOSTATS -O4 $(CINCL)
#CFLAGS=-g -DDDT $(CINCL)
#CFLAGS=-g -march=pentium3 -O3 $(CINCL)
#CFLAGS=-g $(CINCL) -DNDEBUG
#CFLAGS=-g -pg $(CINCL)

LFLAGS=-rdynamic -lgdbm -lcgi6 `curl-config --libs` -llua -lm
#LFLAGS= lcgi5 
#LFLAGS=-ldb -lcgi5 -pg
# For Solaris, use this line
#LFLAGS=-ldb -lcgi5 -lsocket -lnsl

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

all: $(HOSTDIR)/boston

$(HOSTDIR)/boston : $(HOSTDIR)/addutil.o	\
		$(HOSTDIR)/authenticate.o	\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/cgi_main.o		\
		$(HOSTDIR)/cli_main.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/main.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/conversion.o		\
		$(HOSTDIR)/callbacks.o		\
		$(HOSTDIR)/wbtum.o		\
		$(HOSTDIR)/backend.o		\
		$(HOSTDIR)/blogutil.o		\
		$(HOSTDIR)/entity-conversion.o	\
		$(HOSTDIR)/facebook.o		\
		$(HOSTDIR)/be2.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/addutil.o		\
		$(HOSTDIR)/authenticate.o	\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/cgi_main.o		\
		$(HOSTDIR)/cli_main.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/main.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/conversion.o		\
		$(HOSTDIR)/callbacks.o		\
		$(HOSTDIR)/wbtum.o		\
		$(HOSTDIR)/backend.o		\
		$(HOSTDIR)/blogutil.o		\
		$(HOSTDIR)/facebook.o		\
		$(HOSTDIR)/entity-conversion.o	\
		$(HOSTDIR)/be2.o		\
		$(LFLAGS) 
	$(SETUID) $(HOSTDIR)/boston

#######################################################################
#
# Individual files
#
########################################################################

$(HOSTDIR)/test_main.o : src/test_main.c
	$(CC) $(CFLAGS) -c -o $@ src/test_main.c

$(HOSTDIR)/main.o : src/main.c
	$(CC) $(CFLAGS) -c -o $@ src/main.c

$(HOSTDIR)/backend.o : src/backend.c
	$(CC) $(CFLAGS) -c -o $@ src/backend.c

$(HOSTDIR)/cli_main.o : src/cli_main.c
	$(CC) $(CFLAGS) -c -o $@ src/cli_main.c

$(HOSTDIR)/cgi_main.o : src/cgi_main.c
	$(CC) $(CFLAGS) -c -o $@ src/cgi_main.c

$(HOSTDIR)/authenticate.o : src/authenticate.c
	$(CC) $(CFLAGS) -c -o $@ src/authenticate.c

$(HOSTDIR)/array.o : src/array.c
	$(CC) $(CFLAGS) -c -o $@ src/array.c

$(HOSTDIR)/addutil.o : src/addutil.c
	$(CC) $(CFLAGS) -c -o $@ src/addutil.c

$(HOSTDIR)/callbacks.o : src/callbacks.c
	$(CC) $(CFLAGS) -c -o $@ src/callbacks.c

$(HOSTDIR)/bp.o : src/bp.c
	$(CC) $(CFLAGS) -c -o $@ src/bp.c

$(HOSTDIR)/addentry.o : src/addentry.c
	$(CC) $(CFLAGS) -c -o $@ src/addentry.c

$(HOSTDIR)/globals.o : src/globals.c src/conf.h
	$(CC) $(CFLAGS) -c -o $@ src/globals.c

$(HOSTDIR)/blog.o : src/blog.c src/blog.h src/conf.h 
	$(CC) $(CFLAGS) -c -o $@ src/blog.c

$(HOSTDIR)/conversion.o : src/conversion.c src/conversion.h
	$(CC) $(CFLAGS) -c -o $@ src/conversion.c

$(HOSTDIR)/timeutil.o : src/timeutil.c src/timeutil.h
	$(CC) $(CFLAGS) -c -o $@ src/timeutil.c

$(HOSTDIR)/wbttest.o : src/wbttest.c src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/wbttest.c

$(HOSTDIR)/wbtum.o : src/wbtum.c src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/wbtum.c

$(HOSTDIR)/blogutil.o : src/blogutil.c src/blogutil.h
	$(CC) $(CFLAGS) -c -o $@ src/blogutil.c

$(HOSTDIR)/entity-conversion.o : src/entity-conversion.c
	$(CC) $(CFLAGS) -c -o $@ src/entity-conversion.c

$(HOSTDIR)/facebook.o : src/facebook.c
	$(CC) $(CFLAGS) `curl-config --cflags` -c -o $@ src/facebook.c

$(HOSTDIR)/be2.o : src/be2.c
	$(CC) $(CFLAGS) -c -o $@ $<

#######################################################################
#
# Misc targets
#
#######################################################################

clean :
	/bin/rm -rf $(HOSTDIR)/*.o
	/bin/rm -rf *~ src/*~

tarball:
	(cd .. ; tar czvf /tmp/boston.tar.gz -X boston/.exclude boston/ )

