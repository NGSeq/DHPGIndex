#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>
#include "utils.h"

using std::cerr;
using std::endl;

// We assume input is LZ files using uint64_t for each number.
int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "%s <infile1> <infile2> ... <infileN> \n\n"
        "It merges the local LZ parsings of infile1 to infileN, correcting by adding the cumulated offset. Output is written to merged_phrases.lz\n", argv[0]);
    std::exit(EXIT_FAILURE);
  }
	
	FILE * merged_output = utils::open_file("merged_phrases.lz", "w");

	int n_files = argc - 1;
	uint64_t cumulated_offset = 0;
	for (int file_id = 1; file_id <= n_files; file_id++) {
		uint64_t current_offset = 0;

		uint64_t byte_len = utils::file_size(argv[file_id]);
		if (byte_len % 8 ) {
			cerr << argv[file_id] << " has size " << byte_len << " which is not a multple of 8" << endl;
    	std::exit(EXIT_FAILURE);
		}
		
		size_t seq_len = byte_len/8;
		if (seq_len % 2 ) {
			cerr << argv[file_id] << " has a seq_len " << seq_len << " which is not a multple of 2, expected to hold pos,len PAIRS" << endl;
    	std::exit(EXIT_FAILURE);
		}
	
		uint64_t * buffer = new uint64_t[seq_len];
  	FILE * f_input = utils::open_file(argv[file_id], "r");
  	int retval = std::fread(buffer, sizeof(uint64_t), seq_len, f_input);
		if (retval != seq_len) {
			cerr << "Return value of fread: " << retval << "not expected" << endl;
			exit(EXIT_FAILURE);
		}
		fclose(f_input);
		
		// correct ...
		for (size_t p_i = 0; p_i < (seq_len/2); p_i++) {
			uint64_t pos = buffer[2*p_i];
			uint64_t len = buffer[2*p_i + 1];
			if (len != 0) {
				buffer[2*p_i] += cumulated_offset;  // pos needs to be updated.
				current_offset += buffer[2*p_i + 1]; // len informs us about the output of this block.
			} else {
				current_offset++;
			}
		}
		
		// ...and save to merged_output; using the same array...
  	long fwrite_ret = fwrite(buffer, sizeof(uint64_t), seq_len, merged_output);
  	if (fwrite_ret != seq_len) {
    	fprintf(stderr, "Error: fwrite in line %s of %s returned %ld\n",
       	 STR(__LINE__), STR(__FILE__), fwrite_ret);
    	std::exit(EXIT_FAILURE);
  	}
		delete [] buffer;  // Here I can be smarter in mem-management.
		cumulated_offset += current_offset;
	}
	fclose(merged_output);
}

