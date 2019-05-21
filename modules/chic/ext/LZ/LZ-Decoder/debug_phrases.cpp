#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>

#include "stream.h"
#include "utils.h"

using std::cout;
using std::endl;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s <infile> \n\n"
        "Decode LZ77 encoding stored in <infile>\n"
        "and print phrases to std output", argv[0]);
    std::exit(EXIT_FAILURE);
  }

  stream_reader<uint64_t> *reader =
    new stream_reader<uint64_t>(argv[1]);

  fprintf(stderr, "Input file: %s\n", argv[1]);

  while (!(reader->empty())) {
    long pos = reader->read();
    long len = reader->read();
    cout << pos << " , " << len << endl;
  }

  fprintf(stderr, "DONE\n");
}

