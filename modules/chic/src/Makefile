#include /home/dvalenzu/local_installs/sdsl-lite/Make.helper
#CXX_FLAGS=$(MY_CXX_FLAGS) $(MY_CXX_OPT_FLAGS)
#CXX_FLAGS=$(MY_CXX_FLAGS)
PROJECT_ROOT:= $(dir $(abspath $(lastword $(MAKEFILE_LIST))/..))
include ../ext/sdsl-lite-2.1.1/Make.helper

PARANOID=-pedantic -Wall -Wextra -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel  -Wswitch-default -Wundef -Werror -Winline -Wno-error=unused-parameter -Wno-error=unused-variable

#CXX_FLAGS= -std=c++11 -Wall -Wextra -O0 -ggdb -DPROJECT_ROOT="\"$(PROJECT_ROOT)\""
CXX_FLAGS= -std=c++11 -pthread -Wall -Wextra  -O9 -DNDEBUG -DPROJECT_ROOT="\"$(PROJECT_ROOT)\""

INCLUDES=-isystem$(INC_DIR)
LIB=$(LIB_DIR)/libsdsl.a $(LIB_DIR)/libdivsufsort.a $(LIB_DIR)/libdivsufsort64.a /usr/hdp/3.1.0.0-78/hadoop/lib/native/libhdfs.a /usr/lib/jvm/java-1.8.0-openjdk/jre/lib/amd64/server/libjvm.so -lz
LZ_FILES=../ext/LZ/LZscan/algorithm/lcp.cpp ../ext/LZ/LZscan/algorithm/parse.cpp ../ext/LZ/LZscan/algorithm/divsufsort.c ../ext/LZ/LZscan/algorithm/common.cpp

OBJECTS=utils.o RangeReporting.o BookKeeper.o fasta.o KernelManagerFMI.o KernelManagerBWA.o KernelManagerBowTie2.o HybridLZIndex.o LempelZivParser.o
CHICA_BINS=chic_index chic_align chic_map
CHICO_BINS=build_index load_index

%.o: %.cpp
	@echo " [C++] Compiling $<"
	@$(MY_CXX) $(CXX_FLAGS) $(PARANOID) $(INCLUDES) -c $< -o $@

chica: stats $(OBJECTS) $(CHICA_BINS)
chico: stats $(OBJECTS) $(CHICO_BINS)
all: stats $(OBJECTS) $(CHICA_BINS) $(CHICO_BINS)

stats:
	@echo
	@echo " COMPILING HybridLZIndex"
	@echo " ###################"
	@echo "  * Compiler flags: $(CXX_FLAGS)"
	@echo "  * Include dirs: $(INCLUDES)"
	@echo "  * Root Path: $(PROJECT_ROOT)"
	@echo
	@echo

clean:
	@echo " [CLN] Removing object files"
	@rm -f $(OBJECTS) $(CHICA_BINS) $(CHICO_BINS) endmap

build_index: build_index.cpp $(OBJECTS)
	@echo " [BLD] Building build_index"
	@$(MY_CXX) $(CXX_FLAGS) $(INCLUDES) -o build_index build_index.cpp $(LZ_FILES) $(OBJECTS) $(LIB)

chic_index: chic_index.cpp $(OBJECTS)
	@echo " [BLD] Building chic_index"
	@$(MY_CXX) $(CXX_FLAGS) $(INCLUDES) -o chic_index chic_index.cpp $(LZ_FILES) $(OBJECTS) $(LIB) -L/usr/lib/jvm/java-1.8.0-openjdk/jre/lib/amd64/server/ -ljvm

chic_map: chic_map.cpp $(OBJECTS)
	@echo " [BLD] Building chic_map"
	@$(MY_CXX) $(CXX_FLAGS) $(INCLUDES) -o chic_map chic_map.cpp $(LZ_FILES) $(OBJECTS) $(LIB)

load_index: load_index.cpp $(OBJECTS)
	@echo " [BLD] Building load_index"
	@$(MY_CXX) $(CXX_FLAGS) $(INCLUDES) -o load_index load_index.cpp $(LZ_FILES) $(OBJECTS) $(LIB)

chic_align: chic_align.cpp $(OBJECTS)
	@echo " [BLD] Building chic_align"
	@$(MY_CXX) $(CXX_FLAGS) $(INCLUDES) -o chic_align chic_align.cpp $(LZ_FILES) $(OBJECTS) $(LIB)

types: types.vim
types.vim: *.[ch]
	ctags --c-kinds=gstu -o- *.[ch] |\
		awk 'BEGIN{printf("syntax keyword Type\t")}\
			{printf("%s ", $$1)}END{print ""}' > $@
