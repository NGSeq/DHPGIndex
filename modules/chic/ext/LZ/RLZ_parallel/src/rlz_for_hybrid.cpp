#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stdint.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <omp.h>

#include "common.h"
#include "rlz_plain_tools.hpp"
#include "rlz_fasta_tools.hpp"
#include "../../LZscan/algorithm/lzscan.h"

int main(int argc, char **argv) { 
  bool force_large = true;
  if (argc != 9) {
    fprintf(stderr, "usage: encode [REFERENCE_FILE] [REFERENCE_SIZE] [INPUT] [OUTPUT] [N_CHUNKS] [N_THREADS] [MAX_MEM_MB]\n");
    exit(EXIT_FAILURE);
  }

  char * reference_filename = argv[1];
  size_t reference_len = 1024*1024*(size_t)atoi(argv[2]);
  char * input_filename = argv[3];
  char * output_filename = argv[4];
  size_t n_partitions = (size_t)atoi(argv[5]);
  size_t RLZ_THREADS = (size_t)atoi(argv[6]);
  size_t max_memory_MB = atoi(argv[7]);
  int fasta_flag = atoi(argv[8]);
  assert(fasta_flag == 0 || fasta_flag == 1);

  if (n_partitions < RLZ_THREADS) {
    std::cerr << "n_partitions is less than RLZ_threads.. not very effective... " << std::endl;
    // shall we automatically increase n_partitions ?
  }  

  // actually, this sets an upper limit on number of threads.
#ifdef _OPENMP
  omp_set_num_threads(RLZ_THREADS);
#endif

  FILE * file_out = fopen(output_filename,"w");
  Writer w(file_out);

  size_t file_reference_len = file_size(reference_filename);
  if (file_reference_len < reference_len) {
    std::cout << "Reference file length       : " << file_reference_len << std::endl;
    std::cout << "Prefix size given is longer : " << reference_len << std::endl;
    std::cout << "Adjusting it.:" << endl;
    reference_len = file_reference_len;
  }
  if (reference_len == 0 ) {
    std::cout << "ref given is 0. Assuming a test, using n/2 " << std::endl;
    reference_len = file_reference_len/2; 
  }

  size_t n_factors;
  if (fasta_flag) {
    if (reference_len < INT32_MAX && !force_large) {
      n_factors = RLZFasta::parse_in_external_memory<uint32_t>(input_filename,
                                                               reference_filename,
                                                               reference_len,
                                                               n_partitions,
                                                               max_memory_MB,
                                                               w); 
    } else {
      n_factors = RLZFasta::parse_in_external_memory<uint64_t>(input_filename,
                                                               reference_filename,
                                                               reference_len,
                                                               n_partitions,
                                                               max_memory_MB,
                                                               w); 
    }
  } else { 
    if (reference_len < INT32_MAX && !force_large) {
      n_factors = RLZPlain::parse_in_external_memory<uint32_t>(input_filename,
                                                               reference_filename,
                                                               reference_len,
                                                               n_partitions,
                                                               max_memory_MB,
                                                               w); 
    } else {
      n_factors = RLZPlain::parse_in_external_memory<uint64_t>(input_filename,
                                                               reference_filename,
                                                               reference_len,
                                                               n_partitions,
                                                               max_memory_MB,
                                                               w); 
    }
  }
  std::cout << "N Phrases: " << n_factors << std::endl; 
  std::cout << std::endl; 
  fflush(stdout);
  fflush(stderr);
  return EXIT_SUCCESS;
}
