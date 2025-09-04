CC = gcc
CFLAGS = -g -Wall

SRCS = fixpoint.c tctest.c fixpoint_tests.c
OBJS = $(SRCS:.c=.o)

%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o

fixpoint_tests : $(OBJS)
	$(CC) -o $@ $(OBJS)

.PHONY: solution.zip
solution.zip :
	rm -f $@
	zip -9r $@ Makefile *.h *.c README.txt

clean :
	rm -f *.o

depend.mak :
	touch $@

depend :
	$(CC) $(CFLAGS) -M $(SRCS) > depend.mak

include depend.mak
