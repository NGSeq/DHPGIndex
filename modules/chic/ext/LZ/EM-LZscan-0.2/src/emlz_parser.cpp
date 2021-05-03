/**
 * @file    src/main_encode.cpp
 * @section LICENCE
 *
 * This file is part of EM-LZscan v0.2
 * See: http://www.cs.helsinki.fi/group/pads/
 *
 * Copyright (C) 2013-2016
 *   Juha Karkkainen <juha.karkkainen (at) cs.helsinki.fi>
 *   Dominik Kempa <dominik.kempa (at) gmail.com>
 *   Simon J. Puglisi <simon.puglisi (at) cs.helsinki.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 **/

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <string>
#include <getopt.h>
#include <unistd.h>

#include "uint40.h"
#include "uint48.h"
#include "emlzscan_src/parse.h"


char *program_name;

void usage(int status) {
  printf(
"Usage: %s [OPTION]... FILE\n"
"Compute the LZ77 parsing of FILE.\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"  -h, --help              display this help and exit\n"
"  -m, --mem=LIMIT         limit RAM usage to LIMIT MiB (default: 3584)\n"
"  -o, --output=OUTFILE    specify the output file (default: FILE.lz77)\n",
    program_name);

  std::exit(status);
}

bool file_exists(std::string fname) {
  std::FILE *f = std::fopen(fname.c_str(), "r");
  bool ret = (f != NULL);
  if (f != NULL) std::fclose(f);

  return ret;
}

int main(int argc, char **argv) {
  srand(time(0) + getpid());
  program_name = argv[0];

  static struct option long_options[] = {
    {"help",    no_argument,       NULL, 'h'},
    {"mem",     required_argument, NULL, 'm'},
    {"output",  required_argument, NULL, 'o'},
    {NULL, 0, NULL, 0}
  };

  std::uint64_t ram_use = 3584UL << 20;
  std::string out_fname("");

  // Parse command-line options.
  int c;
  while ((c = getopt_long(argc, argv, "hm:o:", long_options, NULL)) != -1) {
    switch(c) {
      case 'm':
        ram_use = std::atol(optarg) << 20;
        if (ram_use <= 0L) {
          fprintf(stderr, "Error: invalid RAM limit (%ld)\n\n", ram_use);
          usage(EXIT_FAILURE);
        }
        break;
      case 'o':
        out_fname = std::string(optarg);
        break;
      case 'h':
        usage(EXIT_FAILURE);
      default:
        usage(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Error: FILE not provided\n\n");
    usage(EXIT_FAILURE);
  }

  // Parse the text filename.
  std::string input_fname = std::string(argv[optind++]);
  if (optind < argc) {
    fprintf(stderr, "Warning: multiple input files provided. "
    "Only the first will be processed.\n");
  }

  // Set default output filename (if not provided).
  if (out_fname.empty())
    out_fname = input_fname + ".lz77";

  // Check if input exists.
  if (!file_exists(input_fname)) {
    fprintf(stderr, "Error: input file (%s) does not exist\n\n",
        input_fname.c_str());
    usage(EXIT_FAILURE);
  }

  if (file_exists(out_fname)) {
    // Output file exists, should we proceed?
    char *line = NULL;
    std::uint64_t buflen = 0;
    std::int64_t len = 0L;

    do {
      printf("Output file (%s) exists. Overwrite? [y/n]: ",
          out_fname.c_str());
      if ((len = getline(&line, &buflen, stdin)) == -1) {
        fprintf(stderr, "\nError: failed to read answer\n\n");
        usage(EXIT_FAILURE);
      }
    } while (len != 2 || (line[0] != 'y' && line[0] != 'n'));

    if (line[0] == 'n') {
      free(line);
      std::exit(EXIT_FAILURE);
    }
    free(line);
  }

  // Run the parsing algorithm.
  parse<uint48, uint40>(input_fname, out_fname, ram_use);
}
