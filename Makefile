CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic

all: traceroute

icmp.o: icmp.h icmp.c

traceroute.o: traceroute.c
traceroute: traceroute.o icmp.o

distclean:
	rm -f *.o traceroute
clean:
	rm -f *.o
