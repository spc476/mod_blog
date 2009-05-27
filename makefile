
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
#CGILIB=/home/spc/source/cgi
CGILIB=../../cgi
SETUID=chmod 4755
#SETUID=echo >/dev/null

CC=gcc -Wall -pedantic 
CINCL=-I $(CGILIB)/src

#CFLAGS=-g -ansi -Wall -pedantic -Wtraditional -Wpointer-arith -Wshadow -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wcast-qual -Waggregate-return -Wmissing-declarations -Wnested-externs -Winline -W $(CINCL)
#CFLAGS=-DSCREAM -O4 -fomit-frame-pointer  $(CINCL)
#CFLAGS=-O3 -fomit-frame-pointer  -DSCREAM $(CINCL)
#CFLAGS=-O3 -fomit-frame-pointer  $(CINCL)
#CFLAGS=-pg -g -DSCREAM -DNOSTATS -O4 $(CINCL)
#CFLAGS=-g -DDDT $(CINCL)
#CFLAGS=-g -march=pentium3 -O3 $(CINCL)
CFLAGS=-g $(CINCL)
#CFLAGS=-g -pg $(CINCL)

LFLAGS=-lgdbm -L$(CGILIB)/$(HOSTDIR) -lcgi5 
#LFLAGS= -L$(CGILIB)/$(HOSTDIR) -lcgi5 
#LFLAGS=-ldb -L$(CGILIB)/$(HOSTDIR) -lcgi5 -pg
# For Solaris, use this line
#LFLAGS=-ldb -L$(CGILIB)/$(HOSTDIR) -lcgi5 -lsocket -lnsl

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

$(HOSTDIR)/boston : $(HOSTDIR)/addutil.o		\
		$(HOSTDIR)/authenticate.o	\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/callbacks.o		\
		$(HOSTDIR)/cgi_main.o		\
		$(HOSTDIR)/chunk.o		\
		$(HOSTDIR)/cli_main.o		\
		$(HOSTDIR)/conversion.o		\
		$(HOSTDIR)/doi_util.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/main.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/wbtum.o		\
		$(HOSTDIR)/backend.o		\
		$(HOSTDIR)/blogutil.o		\
		$(HOSTDIR)/system.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/addutil.o		\
		$(HOSTDIR)/authenticate.o	\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/callbacks.o		\
		$(HOSTDIR)/cgi_main.o		\
		$(HOSTDIR)/chunk.o		\
		$(HOSTDIR)/cli_main.o		\
		$(HOSTDIR)/conversion.o		\
		$(HOSTDIR)/doi_util.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/main.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/wbtum.o		\
		$(HOSTDIR)/backend.o		\
		$(HOSTDIR)/blogutil.o		\
		$(HOSTDIR)/system.o		\
		$(LFLAGS) 
	$(SETUID) $(HOSTDIR)/boston

###########################################################################
#
# Main programs for mod_blog.
#
###########################################################################

$(HOSTDIR)/bp : $(HOSTDIR)/bp.o			\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/system.o		\
		$(HOSTDIR)/chunk.o		\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/wbtum.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/bp.o			\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/system.o		\
		$(HOSTDIR)/chunk.o		\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/wbtum.o		\
		$(LFLAGS)
	$(SETUID) $(HOSTDIR)/bp

$(HOSTDIR)/addentry : $(HOSTDIR)/addentry.o	\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/system.o		\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/conversion.o		\
		$(HOSTDIR)/doi_util.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/addentry.o		\
		$(HOSTDIR)/blog.o		\
		$(HOSTDIR)/globals.o		\
		$(HOSTDIR)/system.o		\
		$(HOSTDIR)/conversion.o		\
		$(HOSTDIR)/timeutil.o		\
		$(HOSTDIR)/doi_util.o		\
		$(LFLAGS) -lgdbm
	$(SETUID) $(HOSTDIR)/addentry

$(HOSTDIR)/wbttest : $(HOSTDIR)/wbttest.o	\
		$(HOSTDIR)/wbtum.o
	$(CC) $(CFLAGS) -o $@			\
		$(HOSTDIR)/wbttest.o		\
		$(HOSTDIR)/wbtum.o		\
		$(LFLAGS)

#######################################################################
#
# Individual files
#
########################################################################

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

$(HOSTDIR)/chunk.o : src/chunk.c src/chunk.h
	$(CC) $(CFLAGS) -c -o $@ src/chunk.c

$(HOSTDIR)/wbttest.o : src/wbttest.c src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/wbttest.c

$(HOSTDIR)/wbtum.o : src/wbtum.c src/wbtum.h
	$(CC) $(CFLAGS) -c -o $@ src/wbtum.c

$(HOSTDIR)/doi_util.o : src/doi_util.c src/doi_util.h
	$(CC) $(CFLAGS) -c -o $@ src/doi_util.c

$(HOSTDIR)/system.o : src/system.c 
	$(CC) $(CFLAGS) -c -o $@ src/system.c

$(HOSTDIR)/blogutil.o : src/blogutil.c src/blogutil.h
	$(CC) $(CFLAGS) -c -o $@ src/blogutil.c

#######################################################################
#
# Misc targets
#
#######################################################################

clean :
	/bin/rm $(HOSTDIR)/*.o
	/bin/rm src/*~

