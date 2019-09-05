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
  void LoadLZParse(FILE * fp,
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
  
  void GetParseRLZ(char * filename,
                   string lzparse_filename,
                   vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                   int max_memory_MB,
                   int rlz_ref_len_MB,
                   int n_threads);
}

#endif
