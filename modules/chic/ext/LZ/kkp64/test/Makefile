include ../../../sdsl-lite-2.1.1/Make.helper
SHELL = /bin/bash
CC = g++
CPPFLAGS = -O3 -DNDEBUG -funroll-loops -static -Wall -Wextra -Wshadow -pedantic
#CPPFLAGS = -O0 -ggdb  -Wall -Wextra -Wshadow -pedantic

INCLUDES=-isystem$(INC_DIR)
LIB=$(LIB_DIR)/libsdsl.a $(LIB_DIR)/libdivsufsort.a $(LIB_DIR)/libdivsufsort64.a

all: compressKKP3

compressKKP3:
	$(CC) $(CPPFLAGS) $(INCLUDES) -o compressKKP3 compressKKP3.cpp common.cpp $(LIB)

clean:
	/bin/rm -f compressKKP3 *.o
