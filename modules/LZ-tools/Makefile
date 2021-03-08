SHELL = /bin/sh

CC = g++
CPPFLAGS = -Wall -static -Wextra -pedantic -Wshadow -funroll-loops -O3 -DNDEBUG
#CPPFLAGS = -Wall -static -Wextra -pedantic -Wshadow -g2 -ggdb
LIB=/media/dev/dev/panquery/modules/chic/ext/sdsl-local-install/lib/libsdsl.a

all: decode decode32 debug_phrases shrink_ints merge_with_offsets merge_meta_offsets merge_vars merge_limits

merge_limits: merge_limits.cpp
	$(CC) $(CPPFLAGS) -o merge_limits merge_limits.cpp utils.cpp $(LIB)

merge_vars: merge_vars.cpp
	$(CC) $(CPPFLAGS) -o merge_vars merge_vars.cpp utils.cpp

merge_meta_offsets: merge_meta_offsets.cpp
	$(CC) $(CPPFLAGS) -o merge_meta_offsets merge_meta_offsets.cpp utils.cpp

merge_with_offsets: merge_with_offsets.cpp
	$(CC) $(CPPFLAGS) -o merge_with_offsets merge_with_offsets.cpp utils.cpp

decode: decode.cpp
	$(CC) $(CPPFLAGS) -o decode decode.cpp utils.cpp

decode32: decode32.cpp
	$(CC) $(CPPFLAGS) -o decode32 decode32.cpp utils.cpp

shrink_ints: shrink_ints.cpp
	$(CC) $(CPPFLAGS) -o shrink_ints shrink_ints.cpp utils.cpp

debug_phrases:
	$(CC) $(CPPFLAGS) -o debug_phrases debug_phrases.cpp utils.cpp

clean:
	rm -f *.o; rm -f decode debug_phrases decode32 shrink_ints merge_with_offsets;
