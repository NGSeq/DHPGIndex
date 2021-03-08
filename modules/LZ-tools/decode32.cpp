#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>

#include "stream.h"
#include "utils.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "%s <infile> <outfile> <prefix_length> \n\n"
        "Decode LZ77 encoding stored in <infile>\n"
        "and write into <outwrite>, it assumes the <prefix_length was used for construction.>\n", argv[0]);
    std::exit(EXIT_FAILURE);
  }

  stream_reader<uint32_t> *reader =
    new stream_reader<uint32_t>(argv[1]);

  fprintf(stderr, "Input file: %s\n", argv[1]);
  fprintf(stderr, "Output file: %s\n", argv[2]);
  fprintf(stderr, "Prexix size: %s\n", argv[3]);
  
  long prefix_size = (atoi(argv[3]) * (1L << 20));

  unsigned char *prefix= new unsigned char[prefix_size];
  std::ofstream ofs(argv[2]);
  
  if (!ofs.good()) {
    fprintf(stderr, "Error opening %s \n.", argv[2]);
    exit(EXIT_FAILURE);
  }
  

  long ptr = 0, dbg = 0, tot = 0;

  while (!(reader->empty())) {
    uint32_t pos = reader->read();
    uint32_t len = reader->read();

    if (!len) {
      if (ptr < prefix_size) {
        prefix[ptr++] = (unsigned char)pos;
      }
      ofs.put((unsigned char)pos);
    }
    else {
      for (uint32_t j = 0; j < len; ++j) {
        if (ptr < prefix_size) {
          prefix[ptr++] = prefix[pos + j];
        }
        ofs.put(prefix[pos+j]);
      }
    }

    // Print progress.
    tot += len;
    if (tot - dbg > (10 << 20)) {
      dbg = tot;
      fprintf(stderr, "Decompressed %.2LfMiB \r",
          (long double)tot / (1 << 20));
    }
  }

  if (!ofs.good()) {
    fprintf(stderr, "An error occurred with the output file.\n");
  }
  fprintf(stderr, "DONE\n");
  delete[] prefix;
}

