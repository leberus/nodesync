CXX = gcc
CFLAGS = -g 
#CFLAGS = -g -DDEBUG
#CFLAGS = -Wall -g -DDEBUG
nodesync : nodesync.o config.o allocate.o event.o mysignal.o
	cc $(CFLAGS) -o nodesync nodesync.o config.o allocate.o event.o mysignal.o

nodesync.o: nodesync.c
	cc $(CFLAGS) -c nodesync.c
config.o: config.c
	cc $(CFLAGS) -c config.c
allocate.o: allocate.c
	cc $(CFLAGS) -c allocate.c
events.o: event.c
	cc $(CFLAGS) -c event.c
mysignal.o: mysignal.c
	cc $(CFLAGS) -c mysignal.c

clean : 
	rm -f *.o nodesync
