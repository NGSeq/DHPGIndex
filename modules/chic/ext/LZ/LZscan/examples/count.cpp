////////////////////////////////////////////////////////////////////////////////
// count.cpp
//   An simple tool demonstrating the use of LZscan. It counts the number of
//   phrases in the LZ77 factorization of a given file.
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Juha Karkkainen, Dominik Kempa and Simon J. Puglisi
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include <fstream>
#include <string>

#include "../algorithm/lzscan.h"

int main(int argc, char **argv) {
  // Check arguments.
  if (argc != 3) {
    fprintf(stderr,"usage: %s <file> bs\n\n"
                   "Computes LZ-factorization of <file>\n"
                   "bs - block size (in MiB)\n", argv[0]);
    exit(0);
  }

  // Parse block size.
  int bs = atoi(argv[2]);
  if (bs <= 0) {
    fprintf(stderr, "error: %d is not a valid block size.\n", bs);
    exit(1);
  }

  // Read input text.
  unsigned char *X = NULL;
  FILE *f = fopen(argv[1], "r");
  if (!f) { perror(argv[1]); exit(1); }
  fseek(f, 0, SEEK_END);
  int n = ftell(f);
  rewind(f);
  X = new unsigned char[n + 5];
  if (!X) {
    fprintf(stderr, "Error: cannot allocate t.\n");
    exit(1);
  }
  int r = fread(X, 1, n, f);
  if (r != n) {
    fprintf(stderr, "Error: fread error.\n");
    exit(1);
  }
  fclose(f);

  // Run the parsing algorithm.  
  clock_t parsing_start = clock();
  int nphrases = parse(X, n, bs << 20, NULL);
  fprintf(stderr, "Total parsing time: %.2Lf\n",
      ((long double)clock() - parsing_start) / CLOCKS_PER_SEC);
  fprintf(stderr, "Number of nphrases = %d\n", nphrases);

  delete[] X;
  return 0;
}
