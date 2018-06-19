CC = gcc
CFLAGS = -Wall -g -O0
LDFLAGS = -pthread

OBJS = pollsrv.o csapp.o

all: pollsrv

submit:
	submit cs61 votelab `pwd`

pollsrv: csapp.o pollsrv.o

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

pollsrv.o: pollsrv.c
	$(CC) $(CFLAGS) -c pollsrv.c

clean:
	rm -f *~ *.o core pollsrv
