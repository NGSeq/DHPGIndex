SHELL = /bin/sh

ifneq ($(MAKECMDGOALS),clean)
	include ../../../sdsl-lite-2.1.1/Make.helper
endif

INCLUDES=-isystem$(INC_DIR)
LIB=$(LIB_DIR)/libdivsufsort.a $(LIB_DIR)/libdivsufsort64.a

CC = g++
CPPFLAGS = -Wall -Wextra -pedantic -Wshadow -funroll-loops -O3 -DNDEBUG -march=native -std=c++0x -pthread
#CPPFLAGS = -Wall -Wextra -pedantic -Wshadow -g2 -O3 -std=c++0x -pthread

#all: encode decode emlz_parser
all: emlz_parser

decode:
	$(CC) $(CPPFLAGS) -o decode_lz77 main_decode.cpp emlzscan_src/utils.cpp

encode:
	$(CC) $(CPPFLAGS) $(INCLUDES) -o encode_lz77 main_encode.cpp emlzscan_src/utils.cpp $(LIB)

emlz_parser:
	$(CC) $(CPPFLAGS) $(INCLUDES) -o emlz_parser emlz_parser.cpp emlzscan_src/utils.cpp $(LIB)

clean:
	rm -f encode_lz77 decode_lz77 emlz_parser *.o

