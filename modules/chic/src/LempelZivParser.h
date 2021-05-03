/*
	 Copyright 2017, Daniel Valenzuela <dvalenzu@cs.helsinki.fi>

	 This file is part of CHIC aligner.

	 CHIC aligner is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.

	 CHIC aligner is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with CHIC aligner.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LEMPEL_ZIV_PARSER_H_
#define LEMPEL_ZIV_PARSER_H_

#include "./utils.h"
#include "./HybridLZIndex.h"
#include <stdlib.h>
#include <vector>

namespace LempelZivParser {
  void GetLZPhrases(vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                    HybridLZIndex * HY);

// Those functions don't need to be visible. We let them here for simplicity only.
  void LoadLZParsePlain(FILE * fp,
                   vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr);
  void LoadLZParseVbyte(std::string filename,
                   vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr);
  void SaveLZParse(vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                   string lzparse_filename);
  void GetParseLZScan(uchar *seq,
                      size_t seq_len,
                      vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                      int max_memory_MB);
  void GetParseEM(char * filename,
                  string lzparse_filename,
                  vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                  int max_memory_MB);
  
  void GetParseReLZ(char * filename,
                    KernelType kernel_type,
                    string lzparse_filename,
                    vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                    int max_memory_MB,
                    int rlz_ref_len_MB,
                    int maxD,
                    int n_threads);
}

#endif
