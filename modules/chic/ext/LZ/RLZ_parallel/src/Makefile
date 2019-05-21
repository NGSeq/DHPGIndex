#include /home/dvalenzu/local_installs/sdsl-lite/Make.helper
#CXX_FLAGS=$(MY_CXX_FLAGS) $(MY_CXX_OPT_FLAGS) 
#CXX_FLAGS=$(MY_CXX_FLAGS)
PROJECT_ROOT:= $(dir $(abspath $(lastword $(MAKEFILE_LIST))/../../..))
ifneq ($(MAKECMDGOALS),clean)
	include ../../../sdsl-lite-2.1.1/Make.helper
endif

PARANOID=-pedantic -Wall -Wextra -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Winline -Wno-error=unused-parameter -Wno-error=unused-variable

#CXX_FLAGS= -std=c++11 -Wall -Wextra  -O0 -ggdb -DPROJECT_ROOT="\"$(PROJECT_ROOT)\""
#CXX_FLAGS= -std=c++11 -Wall -Wextra  -O0 -ggdb -fopenmp -DPROJECT_ROOT="\"$(PROJECT_ROOT)\""
CXX_FLAGS= -std=c++11 -Wall -Wextra  -O9 -DNDEBUG -fopenmp -DPROJECT_ROOT="\"$(PROJECT_ROOT)\""

INCLUDES=-isystem$(INC_DIR)
LIB=$(LIB_DIR)/libsdsl.a $(LIB_DIR)/libdivsufsort.a $(LIB_DIR)/libdivsufsort64.a
LZ_FILES=../../LZscan/algorithm/lcp.cpp ../../LZscan/algorithm/parse.cpp ../../LZscan/algorithm/divsufsort.c ../../LZscan/algorithm/common.cpp

OBJECTS=
BINS=rlz_for_hybrid 

%.o: %.cpp
	@echo " [C++] Compiling $<"
	@$(MY_CXX) $(CXX_FLAGS) $(PARANOID) $(INCLUDES) -c $< -o $@

all: stats $(OBJECTS) $(BINS)

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
	@rm -f $(OBJECTS) $(BINS) 

rlz_for_hybrid: rlz_for_hybrid.cpp $(OBJECTS)
	@echo " [BLD] Building rlz_for_hybrid"
	@$(MY_CXX) $(CXX_FLAGS) $(INCLUDES) -o rlz_for_hybrid rlz_for_hybrid.cpp $(LZ_FILES) $(OBJECTS) $(LIB)

