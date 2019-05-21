////////////////////////////////////////////////////////////////////////////////
// parse.cpp
//   The implementation of the main parsing function of LZscan.
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
#include <cstring>
#include <cstdlib>

#include <vector>
#include <algorithm>

#include "divsufsort.h"
#include "rmq_tree.h"
#include "bitmap.h"
#include "common.h"

#include "ms_support.h"
#include "match.h"
#include "lzscan.h"

const int skip_threshold = 40;

int parse(unsigned char *x, int n, int max_block_size,
    std::vector<std::pair<int, int> > *ret) {
  int *ms_len = new int[max_block_size];
  int *ms_pos = new int[max_block_size];
  bitmap *phrase_boundary = new bitmap(n); // Marks ends of phrases in text.
  int parsed = 0; // Factorized prefix length.
  int nphrases = 0; // Current parsing size.
  while (parsed < n) {
    #ifdef SHOWTIME
    fprintf(stderr, "Parsed %.1LfMB. Next block:\n", 1.L * parsed / (1 << 20));
    #endif
    int block_size = std::min(max_block_size, n - parsed);
    unsigned char *b = x + parsed; // Current block = b[0 .. block_size - 1].

    rmq_tree *rmq = NULL;
    ms_support *index = NULL;
    new_timer("sa");
    int *SA = new int[block_size];
    divsufsort(b, SA, block_size);
    #ifdef SHOWTIME
    fprintf(stderr, "  Suffix array: %.2Lf\n", elapsed("sa"));
    #endif

    std::fill(ms_len, ms_len + block_size, 0);
    if (parsed > 0) {
      // Compute matching statistics for the current block.
      new_timer("ms");
      index = new ms_support(b, SA, block_size);
      #ifdef SHOWTIME
      fprintf(stderr, "  MS-support: %.2Lf\n", elapsed("ms"));
      int previ = parsed;
      #endif

      // For each i we compute pos, lcp such that x[i..i+lcp) occurs in b,
      // lcp is maximized and x[i..i+lcp) = b[SA[pos]..SA[pos]+lcp). 
      new_timer("stream");
      int phrase_end = parsed - 1, phrase_start = phrase_end;
      while (phrase_start > 0 && !phrase_boundary->get(phrase_start - 1)) 
        --phrase_start;
      int pos = index->dollar, lcp = block_size;
      for (int i = parsed - 1; i >= 0; --i) {
        #ifdef SHOWTIME
        if (previ - i > 1000000) {
          fprintf(stderr, "  Stream: %.1Lf%%. Time %.2Lf (speed: %.2LfMB/s)\r",
              100.L * (parsed - i) / parsed, elapsed("stream"),
              1.L * (parsed - i) / ((1 << 20) * elapsed("stream")));
          previ = i;
        }
        #endif
        index->extend_left(pos, lcp, x[i]);
        if (lcp && ms_len[pos] < lcp) {
          ms_len[pos] = lcp;
          ms_pos[pos] = i;
        }

        // Skipping trick. First find the candidate for enclosing phrase.
        if (i < phrase_start) {
          phrase_end = --phrase_start;
          while (phrase_start && !phrase_boundary->get(phrase_start - 1))
            --phrase_start;
        }

        // If it is long enough and x[i..i+lcp) falls inside, restart
        // matching statistics at the beginning of that phrase.
        if (phrase_end - phrase_start > skip_threshold
            && i + lcp - 1 <= phrase_end) {
          i = phrase_start;
          std::pair<int, int> match_pair = index->longest_prefix(x + i);
          pos = match_pair.first;
          lcp = match_pair.second;
          continue;
        }
      }
      #ifdef SHOWTIME
      fprintf(stderr, "  Stream: 100.0%%. Time: %.2Lf (speed %.2Lf MB/s)\n",
          elapsed("stream"), 1.L * parsed / ((1 << 20) * elapsed("stream")));
      #endif

      // Matching statistics inversion.
      for (int i = 1; i < block_size; ++i)
        if (std::min(ms_len[i - 1], index->LCP[i]) > ms_len[i]) {
          ms_len[i] = std::min(ms_len[i - 1], index->LCP[i]);
          ms_pos[i] = ms_pos[i - 1];
        }
      for (int i = block_size - 2; i >= 0; --i)
        if (std::min(ms_len[i + 1], index->LCP[i + 1]) > ms_len[i]) {
          ms_len[i] = std::min(ms_len[i + 1], index->LCP[i + 1]);
          ms_pos[i] = ms_pos[i + 1];
        }
    }

    // Parse the currect block (taking matching stats into account).
    new_timer("parse");
    int *ISA = parsed ? index->LCP : ms_pos; // Space saving.
    for (int i = 0; i < block_size; ++i) ISA[SA[i]] = i;
    rmq = new rmq_tree(SA, block_size, 7);
    int i = 0, pos, len;
    while(i < block_size) {
      int psv = rmq->psv(ISA[i], i), nsv = rmq->nsv(ISA[i], i);
      parse_phrase(b, block_size, i, psv, nsv, pos, len);
      pos = len ? pos + parsed : b[i];
      if ((ms_len[ISA[i]] > len && len) || (!len && ms_len[ISA[i]])) {
        len = ms_len[ISA[i]];
        pos = ms_pos[ISA[i]];
      }

      // Do not add phrase that can overlap block boundary.
      if (len && i + len == block_size && parsed + block_size < n) break;
      if (ret) ret->push_back(std::make_pair(pos, len));
      i += std::max(1, len);
      phrase_boundary->set(parsed + i - 1);
      ++nphrases;
    }
    #ifdef SHOWTIME
    fprintf(stderr, "  Parse block: %.2Lf\n", elapsed("parse"));
    #endif

    if (i < (block_size + 1) / 2) {
      // The last phrase was long -- compute it using pattern matching.
      std::pair<int, int> lcp_pair = maxlcp(x, n, parsed + i);
      if (ret) ret->push_back(lcp_pair);
      i += std::max(1, lcp_pair.second);
      phrase_boundary->set(parsed + i - 1);
      ++nphrases;
    }
    parsed += i;
    delete rmq;
    delete index;
    delete[] SA;
  }
  delete(phrase_boundary);
  delete[] ms_len;
  delete[] ms_pos;
  return nphrases;
}
