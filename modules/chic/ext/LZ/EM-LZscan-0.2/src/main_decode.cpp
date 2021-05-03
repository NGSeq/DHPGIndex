/**
 * @file    src/main_decode.cpp
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
#include <string>
#include <sys/time.h>

#include "uint48.h"


long double wclock() {
  timeval tim;
  gettimeofday(&tim, NULL);
  return tim.tv_sec + (tim.tv_usec / 1000000.0L);
}

std::FILE *file_open(std::string filename, std::string mode) {
  std::FILE *f = std::fopen(filename.c_str(), mode.c_str());
  if (f == NULL) {
    std::perror(filename.c_str());
    std::exit(EXIT_FAILURE);
  }
  return f;
}

std::uint64_t file_size(std::string filename) {
  std::FILE *f = file_open(filename, "r");
  std::fseek(f, 0, SEEK_END);
  long size = std::ftell(f);
  if (size < 0) {
    std::perror(filename.c_str());
    std::exit(EXIT_FAILURE);
  }
  std::fclose(f);
  return (std::uint64_t)size;
}

template<typename value_type>
void write_to_file(const value_type *src, std::uint64_t length, std::FILE *f) {
  std::uint64_t fwrite_ret = std::fwrite(src, sizeof(value_type), length, f);
  if (fwrite_ret != length) {
    fprintf(stderr, "Error: fwrite failed.\n");
    std::exit(EXIT_FAILURE);
  }
}

template<typename value_type>
void read_from_file(value_type* dest, std::uint64_t length, std::FILE *f) {
  std::uint64_t fread_ret = std::fread(dest, sizeof(value_type), length, f);
  if (fread_ret != length) {
    fprintf(stderr, "\nError: fread failed.\n");
    std::exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s FILE\n"
        "Decode the LZ77 parsing stored in FILE and write on standard output.\n",
        argv[0]);
    std::exit(EXIT_FAILURE);
  }

  std::string parsing_filename = argv[1];
  typedef uint48 textoff_t;

  std::uint64_t n_phrases = file_size(parsing_filename) / (2 * sizeof(textoff_t));
  std::uint64_t text_length = 0;
  std::FILE *f_parsing = file_open(parsing_filename, "r");
  {
    textoff_t temp;
    read_from_file(&temp, 1, f_parsing);
    text_length = temp;
  }

  fprintf(stderr, "Input file =  %s\n", argv[1]);
  fprintf(stderr, "Text length = %lu\n", text_length);
  fprintf(stderr, "Number of phrases = %lu\n", n_phrases);
  fprintf(stderr, "sizeof(textoff_t) = %lu\n", sizeof(textoff_t));
  fprintf(stderr, "\n");

  // Allocate the text.
  std::uint8_t *text = new std::uint8_t[text_length];

  // Stream the phrases and reconstruct the text in RAM.
  {
    fprintf(stderr, "Decode: ");
    long double start = wclock();

    static const std::uint64_t buf_size = (1L << 20);
    textoff_t *buf = new textoff_t[2 * buf_size];

    std::uint64_t phrases_decoded = 0;
    std::uint64_t symbols_decoded = 0;

    while (phrases_decoded < n_phrases) {
      std::uint64_t buf_filled = std::min(n_phrases - phrases_decoded, buf_size);
      read_from_file(buf, buf_filled * 2, f_parsing);

      for (std::uint64_t i = 0; i < buf_filled; ++i) {
        std::uint64_t phrase_src = buf[i * 2];
        std::uint64_t phrase_len = buf[i * 2 + 1];

        if (phrase_len == 0) text[symbols_decoded++] = (std::uint8_t)phrase_src;
        else {
          for (std::uint64_t j = 0; j < phrase_len; ++j)
            text[symbols_decoded++] = text[phrase_src + j];
        }
      }

      phrases_decoded += buf_filled;
    }

    delete[] buf;

    // Print runtime.
    fprintf(stderr, "%.2Lfs\n", wclock() - start);
  }

  // Write text to file.
  {
    long double start = wclock();
    fprintf(stderr, "Write to file: ");
    write_to_file(text, text_length, stdout);
    fprintf(stderr, "%.2Lfs\n", wclock() - start);
  }

  std::fclose(f_parsing);
  delete[] text;
}
