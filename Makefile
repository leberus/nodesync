CXX = gcc
CXXFLAGS = -Wall -g
nodesync : nodesync.o config.o
	cc -o nodesync nodesync.o config.o

nodesync.o: nodesync.c
	cc -c nodesync.c
config.o: config.c
	cc -c config.c


clean : 
	rm -f *.o nodesync
