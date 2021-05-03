////////////////////////////////////////////////////////////////////////////////
// count.cpp
//   An example tool computing the size of LZ77 parsing of a given file.
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

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cstdio>

//#include <divsufsort.h>
#include <divsufsort64.h>

#include "../algorithm/kkp.hpp"
#include "common.h"

using std::pair;
using std::vector;
using std::string;

int main(int argc, char **argv) {
  // Check arguments.
  if(argc != 3 ) {
    std::cerr << "usage: " << argv[0] << " infile  outfile\n\n"
      << "Computes the size of LZ77 parsing of infile using       \n"
      << "  kkp3  -- the fastest, uses 13n bytes (default)        \n" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Read the text and its suffix array.
  unsigned char *text;
  int64_t length;
  read_text(argv[1], text, length);
  string file_out = string(argv[2]);
  // Compute the size of LZ77 parsing.
  int64_t *sa = new int64_t[length + 2];
  if (!sa) {
    std::cerr << "\nError: allocating " << length << " words failed\n";
    std::exit(EXIT_FAILURE);
  }
  //divsufsort(text, sa, length);
  divsufsort64(text, sa, length);
  int nphrases;

  vector<pair<int64_t, int64_t> > phrases;
  nphrases = kkp3(text, sa, length, &phrases);


  FILE* fp = fopen(file_out.c_str(), "w");
  //Writer w(file_out);

  std::cerr << "Number of phrases = " << nphrases << std::endl;

  // TODO: one fwrite
  for (int i = 0; i < nphrases; i++) {
    int64_t val = phrases[i].first;
    if (1 != fwrite(&val, sizeof(int64_t), 1, fp)) {
      std::cerr << "Error flushing phrases " << std::endl;
    }
    val = phrases[i].second;
    if (1 != fwrite(&val, sizeof(int64_t), 1, fp)) {
      std::cerr << "Error flushing phrases " << std::endl;
    }
  }
  fclose(fp);

  // Clean up.
  if (sa) delete[] sa;
  delete[] text;
  return EXIT_SUCCESS;
  
}
