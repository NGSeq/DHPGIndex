BSHELL = /bin/bash
CC = g++
CPPFLAGS = -O3 -DNDEBUG -funroll-loops -static -Wall -Wextra -Wshadow -pedantic

all: count

count:
	$(CC) $(CPPFLAGS) -o count -DSHOWTIME count.cpp ../algorithm/lcp.cpp \
	../algorithm/parse.cpp ../algorithm/divsufsort.c ../algorithm/common.cpp

clean:
	/bin/rm -f count *.o
