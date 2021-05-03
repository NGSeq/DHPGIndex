SHELL = /bin/sh

CC = g++
CPPFLAGS = -Wall  -Wextra -pedantic -Wshadow -funroll-loops -O3 -DNDEBUG
#CPPFLAGS = -Wall  -Wextra -pedantic -Wshadow -g2

all: decode debug_phrases

decode:
	$(CC) $(CPPFLAGS) -o decode decode.cpp utils.cpp

debug_phrases:
	$(CC) $(CPPFLAGS) -o debug_phrases debug_phrases.cpp utils.cpp

clean:
	rm -f *.o; rm -f decode debug_phrases;
