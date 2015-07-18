
CC      = gcc -std=c99
CFLAGS  = -g -Wall -Wextra -pedantic -I../src
LDFLAGS = -g
LDLIBS  = 

tumtest : test.o wbtum.o timeutil.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

wbtum.o : wbtum.c wbtum.h
test.o  : wbtum.h
timeutil.o : ../src/timeutil.c ../src/timeutil.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	$(RM) *~ *.o core.* tumtest
