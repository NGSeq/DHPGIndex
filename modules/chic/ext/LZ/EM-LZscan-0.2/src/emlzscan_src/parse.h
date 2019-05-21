/**
 * @file    src/emlzscan_src/parse.h
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

#ifndef __EMLZSCAN_SRC_PARSE_H_INCLUDED
#define __EMLZSCAN_SRC_PARSE_H_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <divsufsort.h>
#include <divsufsort64.h>

#include "ms_support.h"
#include "match.h"
#include "utils.h"
#include "rmq_tree.h"
#include "compute_lcp.h"
#include "io/bidirectional_accessor.h"
#include "io/backward_stream_reader.h"
#include "io/stream_writer.h"


namespace emlzscan_private {

template<typename textoff_t, typename blockoff_t>
void parse(std::string text_filename, std::string out_filename, std::uint64_t ram_use) {
  static const std::uint64_t min_long_phrase_len = 40;
  static const std::uint64_t ms_support_block_size = 1024;
  static const std::uint64_t k_sigma = 256;

  // Turn paths absolute.
  text_filename = utils::absolute_path(text_filename);
  out_filename = utils::absolute_path(out_filename);

  long double start = utils::wclock();
  std::uint64_t n_phrases = 0;
  std::uint64_t total_io_volume = 0;
  std::uint64_t text_length = utils::file_size(text_filename);
  std::uint64_t max_block_size = ram_use / (24 / (ms_support_block_size / k_sigma) + 2 + 3 * sizeof(blockoff_t) + 2 * sizeof(textoff_t));
  std::string long_phrases_filename = out_filename + ".long_phrases." + utils::random_string_hash();

  fprintf(stderr, "Text filename = %s\n", text_filename.c_str());
  fprintf(stderr, "Output filename = %s\n", out_filename.c_str());
  fprintf(stderr, "RAM use = %lu (%.2LfMiB)\n", ram_use, (1.L * ram_use) / (1L << 20));
  fprintf(stderr, "Max block size = %lu (%.2LfMiB)\n", max_block_size, (1.L * max_block_size) / (1L << 20));
  fprintf(stderr, "sizeof(textoff_t) = %lu\n", sizeof(textoff_t));
  fprintf(stderr, "sizeof(blockoff_t) = %lu\n", sizeof(blockoff_t));
  fprintf(stderr, "\n");

  // Create text reader and phrase writer.
  typedef bidirectional_accessor<std::uint8_t> text_accessor_type;
  text_accessor_type *text_accessor = new text_accessor_type(text_filename);
  typedef stream_writer<uint64_t> phrase_writer_type;
  phrase_writer_type *phrase_writer = new phrase_writer_type(out_filename);

  // Write header.
  //phrase_writer->write(text_length);

  // Parse blocks left to right.
  std::uint64_t block_beg = 0;
  while (block_beg < text_length) {
    std::uint64_t block_end = std::min(block_beg + max_block_size, text_length);
    std::uint64_t block_size = block_end - block_beg;

    fprintf(stderr, "Process block [%lu..%lu):\n", block_beg, block_end);

    // Read block.
    std::uint8_t *block = new std::uint8_t[block_size];
    {
      fprintf(stderr, "  Read block: ");
      long double read_start = utils::wclock();
      utils::read_at_offset(block, block_beg, block_size, text_filename);
      total_io_volume += block_size;
      long double read_time = utils::wclock() - read_start;
      fprintf(stderr, "%.2Lfs\n", read_time);
    }

    // Compute suffix array of the block.
    blockoff_t *block_sa = new blockoff_t[block_size];
    {
      fprintf(stderr, "  Compute SA of block: ");
      long double sufsort_start = utils::wclock();
      if (block_size < 2147483648UL) {
        std::int32_t *block_sa_32 = new std::int32_t[block_size];
        divsufsort(block, block_sa_32, block_size);
        for (std::uint64_t j = 0; j < block_size; ++j)
          block_sa[j] = block_sa_32[j];
        delete[] block_sa_32;
      } else {
        std::int64_t *block_sa_64 = new std::int64_t[block_size];
        divsufsort64(block, block_sa_64, block_size);
        for (std::uint64_t j = 0; j < block_size; ++j)
          block_sa[j] = block_sa_64[j];
        delete[] block_sa_64;
      }
      long double sufsort_time = utils::wclock() - sufsort_start;
      fprintf(stderr, "%.2Lfs\n", sufsort_time);
    }

    // Allocate arrays holding matching statistics.
    textoff_t *ms_pos = new textoff_t[block_size];
    textoff_t *ms_len = new textoff_t[block_size];

    // Compute matching statistics for the current block.
    std::fill(ms_len, ms_len + block_size, (textoff_t)0);
    if (block_beg > 0) {

      // Allocate arrays.
      blockoff_t *block_lcp = new blockoff_t[block_size + 1];
      blockoff_t *block_rank = new blockoff_t[block_size];

      // Compute LCP array of the block.
      {
        fprintf(stderr, "  Compute LCP of block: ");
        long double lcp_start = utils::wclock();
        compute_lcp(block, block_size, block_sa, block_lcp, block_rank);
        block_lcp[block_size] = 0;  // XXX
        long double lcp_time = utils::wclock() - lcp_start;
        fprintf(stderr, "%.2Lfs\n", lcp_time);
      }

      // Compute matching statistic support.
      fprintf(stderr, "  Compute MS-support: ");
      long double compute_ms_support_start = utils::wclock();
      typedef ms_support<blockoff_t, ms_support_block_size> ms_support_type;
      ms_support_type *index = new ms_support_type(block, block_sa, block_lcp, block_rank, block_size);
      long double compute_ms_support_time = utils::wclock() - compute_ms_support_start;
      fprintf(stderr, "%.2Lfs\n", compute_ms_support_time);

      // Initialize backward stream reader of long phrases.
      typedef backward_stream_reader<textoff_t> long_phrases_reader_type;
      long_phrases_reader_type *long_phrases_reader =
        new long_phrases_reader_type(long_phrases_filename);

      // For each i we compute pos, lcp such that x[i..i+lcp) occurs in block,
      // lcp is maximized and x[i..i+lcp) = block[SA[pos]..SA[pos]+lcp). 
      std::uint64_t phrase_length = 0;
      std::uint64_t phrase_start = 0;
      if (long_phrases_reader->empty()) phrase_length = 0;
      else {
        phrase_length = long_phrases_reader->read();
        phrase_start = long_phrases_reader->read();
      }

      fprintf(stderr, "  Stream: \r");
      long double stream_start = utils::wclock();
      std::uint64_t pos = index->m_dollar;
      std::uint64_t lcp = block_size;

      for (std::uint64_t i = block_beg; i > 0; ) {
        --i;
        std::uint8_t next_symbol = text_accessor->access(i);
        index->extend_left(pos, lcp, next_symbol);

        if (lcp > 0 && lcp > (std::uint64_t)ms_len[pos]) {
          ms_len[pos] = lcp;
          ms_pos[pos] = i;
        }

        // Skipping trick. First find the candidate for enclosing phrase.
        if (phrase_length > 0 && i < phrase_start) {
          if (long_phrases_reader->empty()) phrase_length = 0;
          else {
            phrase_length = long_phrases_reader->read();
            phrase_start = long_phrases_reader->read();
          }
        }

        // If it is long enough and x[i..i+lcp) falls inside, restart
        // matching statistics at the beginning of that phrase.
        if (phrase_length > 0 && i + lcp <= phrase_start + phrase_length) {
          i = phrase_start;
          std::pair<std::int64_t, std::int64_t> match_pair = index->longest_prefix(i, text_accessor);
          pos = match_pair.first;
          lcp = match_pair.second;
          continue;
        }
      }

      // Print summary.
      long double stream_time = utils::wclock() - stream_start;
      fprintf(stderr, "  Stream: 100.0%%, time = %.2Lf, I/O = %.2LfMB/s\n",
          stream_time, ((1.L * block_beg) / (1L << 20)) / stream_time);

      // Invert matching statistics.
      {
        fprintf(stderr, "  Invert matching statistics: ");
        long double invert_ms_start = utils::wclock();
        for (std::uint64_t i = 1; i < block_size; ++i) {
          if (std::min((std::uint64_t)ms_len[i - 1], (std::uint64_t)block_lcp[i]) > (std::uint64_t)ms_len[i]) {
            ms_len[i] = std::min((std::uint64_t)ms_len[i - 1], (std::uint64_t)block_lcp[i]);
            ms_pos[i] = ms_pos[i - 1];
          }
        }

        for (std::uint64_t i = block_size - 1; i > 0; --i) {
          if (std::min((std::uint64_t)ms_len[i], (std::uint64_t)block_lcp[i]) > (std::uint64_t)ms_len[i - 1]) {
            ms_len[i - 1] = std::min((std::uint64_t)ms_len[i], (std::uint64_t)block_lcp[i]);
            ms_pos[i - 1] = ms_pos[i];
          }
        }

        long double invert_ms_time = utils::wclock() - invert_ms_start;
        fprintf(stderr, "%.2Lfs\n", invert_ms_time);
      }

      total_io_volume += long_phrases_reader->bytes_read();

      // Clean up.
      delete[] block_lcp;
      delete[] block_rank;
      delete index;
      delete long_phrases_reader;
    }

    // Create the long phrase writer in the append mode.
    typedef stream_writer<textoff_t> long_phrase_writer_type;
    long_phrase_writer_type *long_phrase_writer = new
      long_phrase_writer_type(long_phrases_filename, (8UL << 20), 4, "a");

    // Parse the currect block
    std::uint64_t parsed = 0;
    {
      fprintf(stderr, "  Parse block: ");
      long double parse_block_start = utils::wclock();

      // Compute inverse suffix array.
      blockoff_t *block_isa = new blockoff_t[block_size];
      for (std::uint64_t i = 0; i < block_size; ++i)
        block_isa[block_sa[i]] = i;

      // Initialize PSV/NSV for suffix array.
      typedef rmq_tree<blockoff_t> rmq_type;
      rmq_type *rmq = new rmq_type(block_sa, block_size);

      while (parsed < block_size) {
        // Compute next phrase.
        std::uint64_t psv_index = rmq->psv(block_isa[parsed], parsed);
        std::uint64_t nsv_index = rmq->nsv(block_isa[parsed], parsed);
        std::uint64_t len = 0;
        std::uint64_t pos = 0;

        if (nsv_index != block_size && psv_index != block_size) {
          std::uint64_t psv_value = block_sa[psv_index];
          std::uint64_t nsv_value = block_sa[nsv_index];
          while (psv_value + len < block_size && nsv_value + len < block_size &&
              block[psv_value + len] == block[nsv_value + len]) ++len;
          if (parsed + len < block_size && block[parsed + len] == block[psv_value + len]) {
            ++len;
            while (parsed + len < block_size && block[parsed + len] == block[psv_value + len]) ++len;
            pos = psv_value;
          } else {
            while (parsed + len < block_size && block[parsed + len] == block[nsv_value + len]) ++len;
            pos = nsv_value;
          }
        } else if (psv_index != block_size) {
          std::uint64_t psv_value = block_sa[psv_index];
          while (parsed + len < block_size && block[parsed + len] == block[psv_value + len]) ++len;
          pos = psv_value;
        } else if (nsv_index != block_size) {
          std::uint64_t nsv_value = block_sa[nsv_index];
          while (parsed + len < block_size && block[parsed + len] == block[nsv_value + len]) ++len;
          pos = nsv_value;
        }

        if (len == 0) pos = block[parsed];
        else pos += block_beg;

        if (((std::uint64_t)ms_len[block_isa[parsed]] > len && len > 0) || (len == 0 && (std::uint64_t)ms_len[block_isa[parsed]] > 0)) {
          len = ms_len[block_isa[parsed]];
          pos = ms_pos[block_isa[parsed]];
        }

        // Do not add phrase that can might overlap block boundary.
        if (len > 0 && parsed + len == block_size && block_end < text_length)
          break;

        phrase_writer->write(pos);
        phrase_writer->write(len);

        if (len > min_long_phrase_len) {
          long_phrase_writer->write(block_beg + parsed);
          long_phrase_writer->write(len);
        }

        parsed += std::max(1UL, len);
        ++n_phrases;
      }

      // Clean up.
      delete rmq;
      delete[] block_isa;

      // Print summary.
      long double parse_block_time = utils::wclock() - parse_block_start;
      long double elapsed_since_beg = utils::wclock() - start;

      fprintf(stderr, "%.2Lfs\n", parse_block_time);
      fprintf(stderr, "  Current progress: %.1Lf%%, elapsed = %.0Lfs, z = %lu, total I/O vol = %.2Lfn\n",
          (100.L * block_beg) / text_length, elapsed_since_beg, n_phrases,
          (1.L * (total_io_volume + text_accessor->bytes_read() + phrase_writer->bytes_written())) / text_length);
    }

    delete[] block_sa;
    delete[] block;
    delete[] ms_pos;
    delete[] ms_len;

    // Handle long phrases.
    if (parsed < (block_size + 1) / 2) {
      fprintf(stderr, "  Handle long phrases: ");
      long double handle_long_phrase_start = utils::wclock();
      std::uint64_t big_block_size = std::min(text_length - (block_beg + parsed), ram_use);
      block = new std::uint8_t[big_block_size];

      utils::read_at_offset(block, block_beg + parsed, big_block_size, text_filename);
      std::reverse(block, block + big_block_size);
      std::pair<std::uint64_t, std::uint64_t> pp = pattern_matching(block_beg + parsed,
          text_length, block, big_block_size, text_accessor);

      // Phrases longer than big_block_size and currently
      // broken into shorter phrases, but they could
      // potentially be handles separatelly here.
      if (pp.second == big_block_size) {}

      // Store the computed phrase and continue.
      phrase_writer->write(pp.first);
      phrase_writer->write(pp.second);

      if (pp.second > min_long_phrase_len) {
        long_phrase_writer->write(block_beg + parsed);
        long_phrase_writer->write(pp.second);
      }

      ++n_phrases;
      parsed += pp.second;
      delete[] block;

      long double handle_long_phrase_time = utils::wclock() - handle_long_phrase_start;
      fprintf(stderr, "%.2Lfs\n", handle_long_phrase_time);
    }

    total_io_volume += long_phrase_writer->bytes_written();

    delete long_phrase_writer;
    block_beg += parsed;
  }

  total_io_volume += text_accessor->bytes_read();
  total_io_volume += phrase_writer->bytes_written();

  // Clean up.
  delete text_accessor;
  delete phrase_writer;
  if (utils::file_exists(long_phrases_filename))
    utils::file_delete(long_phrases_filename);

  // Print summary.
  long double total_time = utils::wclock() - start;
  fprintf(stderr, "\n\nComputation finished. Summary:\n");
  fprintf(stderr, "  elapsed time = %.2Lfs (%.3Lfs/MiB of text)\n",
      total_time, total_time / ((1.L * text_length) / (1L << 20)));
  fprintf(stderr, "  speed = %.2LfMiB of text/s\n",
      ((1.L * text_length) / (1L << 20)) / total_time);
  fprintf(stderr, "  I/O volume = %lubytes (%.2Lfbytes/input symbol)\n",
      total_io_volume, (1.L * total_io_volume) / text_length);
  fprintf(stderr, "  Number of phrases = %lu\n", n_phrases);
  fprintf(stderr, "  Average phrase length = %.3Lf\n", (1.L * text_length) / n_phrases);
}

}  // namespace emlzscan_private

template<typename textoff_t, typename blockoff_t>
void parse(std::string text_filename, std::string out_filename, std::uint64_t ram_use) {
  emlzscan_private::parse<textoff_t, blockoff_t>(text_filename, out_filename, ram_use);
}

#endif  // __EMLZSCAN_SRC_PARSE_H_INCLUDED
