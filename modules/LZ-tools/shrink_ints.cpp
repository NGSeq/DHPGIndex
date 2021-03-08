#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>
//#include <stdio>

#include "stream.h"
#include "utils.h"

using std::cerr;
using std::endl;

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "%s <infile> <outfile> \n\n"
        "shrink_ints will encode the 64 ints from <infile> into 32 bits ints in <outfile>\n", argv[0]);
    std::exit(EXIT_FAILURE);
  }
	
	uint64_t byte_len = utils::file_size(argv[1]);
	if (byte_len % 8 ) {
		cerr << argv[1] << " has size " << byte_len << " which is not a multple of 8" << endl;
    std::exit(EXIT_FAILURE);
	}
	size_t seq_len = byte_len/8;
	
	uint64_t * input = new uint64_t[seq_len];
  FILE * f_input = utils::open_file(argv[1], "r");
  int retval = std::fread(input, sizeof(uint64_t), seq_len, f_input);
	cerr << "Return value of fread" << retval << endl;
	fclose(f_input);
	
	uint32_t * output = new uint32_t[seq_len];
	for (size_t i = 0; i < seq_len; i++) {
		output[i] = (uint32_t)input[i];
	}
  FILE * f_output = utils::open_file(argv[2], "w");
  long fwrite_ret = fwrite(output, sizeof(uint32_t), seq_len, f_output);
  if (fwrite_ret != seq_len) {
    fprintf(stderr, "Error: fwrite in line %s of %s returned %ld\n",
        STR(__LINE__), STR(__FILE__), fwrite_ret);
    std::exit(EXIT_FAILURE);
  }
	fclose(f_output);

	delete [] input;
	delete [] output;
}

