#ifndef PARSEREF_H
#define PARSEREF_H
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stdint.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "dictionary.h"
#include "dictionary_fasta.h"
#include "common.h"
#include "../../LZscan/algorithm/lzscan.h"
#include "../../kkp64/algorithm/kkp.hpp"

namespace ReferenceParser{

using std::pair;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::string;


void LoadLZParse(FILE * fp,
                 vector<pair<uint64_t, uint>> * lz_phrases_ptr) {
  // We assume 64 bits per number
  fseek(fp, 0, SEEK_END);
  size_t file_len = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);
  // beware this len is the numbers of bytes.
  cout << "len in bytes: " << file_len;
  file_len = file_len*sizeof(uint8_t)/sizeof(uint64_t);
  cout << "len in nums: " << file_len;
  assert(file_len % 2 == 0);
  uint64_t * buff = new uint64_t[file_len];
  if (file_len != fread(buff, sizeof(uint64_t), file_len, fp)) {
    cout << stderr << "Error reading string from file" << endl;
    exit(1);
  }
  size_t n_phrases = file_len/2;

  lz_phrases_ptr->reserve(n_phrases);
  for (size_t i = 0; i < n_phrases; i++) {
    uint64_t  pos = buff[2*i];
    uint len = (uint)buff[2*i+1];
    lz_phrases_ptr->push_back(pair<uint64_t, uint>(pos, len));
  }
  delete [] buff;
}

void CallParseEM(char * filename, 
                vector<pair<uint64_t, uint>> * lz_phrases_ptr, 
                int max_memory_MB) {
#ifndef PROJECT_ROOT
  cout << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(-1);
#else
  char * command_parse= new char[1024];

  char lz_parse_filename[200];
  sprintf(lz_parse_filename, "%s.lzparse",filename);
  char lz_parse_log_filename[200];
  sprintf(lz_parse_log_filename, "%s.log_em_parse",filename);
  
  sprintf(command_parse,
          "%s/ext/EM-LZscan-0.2/src/emlz_parser %s --mem=%i --output=%s > %s 2>&1",
          PROJECT_ROOT,
          filename,
          max_memory_MB,
          lz_parse_filename,
          lz_parse_log_filename);

  printf("We wil call:\n %s\n command.", command_parse);
  if (system(command_parse)) {
    cout << "Command failed. " << endl;
    exit(-1);
  }

  FILE * lz_infile = fopen(lz_parse_filename, "r");
  if (!lz_infile) {
    cout << "Error opening file" << filename << "for readind." << endl;
    exit(EXIT_FAILURE);
  }
  LoadLZParse(lz_infile, lz_phrases_ptr);
  fclose(lz_infile);
  delete [] command_parse;
#endif
}

size_t em_parse_ref(char * filename, 
                  Writer &w,
                  int max_memory_MB) {
  cout << "Using EM LZScan..." << endl;
  vector<pair<uint64_t,uint>> ref_parsing;
  CallParseEM(filename, 
              &ref_parsing, 
              max_memory_MB);
  
  for (size_t i = 0; i < ref_parsing.size(); i++) {
    w.write(ref_parsing[i].first, 64);
    w.write(ref_parsing[i].second, 64);
  }
  return (size_t)ref_parsing.size();
}

size_t lz_parse_ref(uint8_t * seq,
                    size_t seq_len,
                    Writer &w,
                    int max_memory_MB) {
  cout << "Using IN-Memory LZScan..." << endl;
  vector<pair<int, int>> ref_parsing;
  int n_phrases = parse(seq, 
                        seq_len, 
                        max_memory_MB << 20, 
                        &ref_parsing);
  for (int i = 0; i < n_phrases; i++) {
    w.write(ref_parsing[i].first, 64);
    w.write(ref_parsing[i].second, 64);
  }
  return (size_t)n_phrases;
}

template <typename sa_t>
size_t kkp_parse_ref(uint8_t * seq,
                     sa_t* sa,
                     size_t seq_len,
                     Writer &w) {
  cout << "Using IN-Memory KKP3..." << endl;
  fflush(stdout); 
  vector<pair<int64_t, int64_t>> ref_parsing;
  int64_t * tmp_sa = new int64_t[seq_len+2];
  for (size_t i = 0; i < seq_len; i++) tmp_sa[i] = sa[i];
  cout << "about to call KP3..." << endl;
  fflush(stdout); 
  int n_phrases = kkp3(seq,
                       (int64_t*)tmp_sa,
                       seq_len, 
                       &ref_parsing);
  cout << "KP3 done, printing..." << endl;
  fflush(stdout);
  // TODO: do it in one pass...
  for (int i = 0; i < n_phrases; i++) {
    w.write(ref_parsing[i].first, 64);
    w.write(ref_parsing[i].second, 64);
  }
  cout << "KP3 done and printed" << endl;
  fflush(stdout); 
  return (size_t)n_phrases;
}


//TODO: Dictionary and DictionaryFA should be refactored, and then the following two 
// semi-redundant methods will be refactored too.

template <typename sa_t>
size_t parse_ref(Dictionary<sa_t> &d,
                 char * reference_filename,
                 Writer &w,
                 int max_memory_MB) {
  cout << "Parsing the ref..." << endl;
  long double start_time= wclock();
  size_t n_factors; 
  //long max_block_size = (atoi(argv[2]) * (1L << 20)) / 29L;
  size_t EMLZ_limit_MB = (29L*(INT32_MAX/2))/(1L<<20);
  size_t EMLZ_mem_MB = std::min(EMLZ_limit_MB, (size_t)max_memory_MB);
  if (sizeof(sa_t) <= 4) {
    n_factors = lz_parse_ref(d.d, d.n, w, EMLZ_mem_MB);
  } else if (sizeof(sa_t) == 8) {
    bool enough_memory = false;  // TODO propper calculation.
    if (enough_memory) {
      n_factors = kkp_parse_ref(d.d, d.sa, d.n, w);
    } else {
      n_factors = em_parse_ref(reference_filename, w, EMLZ_mem_MB);
    }
  } else {
    cout << "Unfamiliar sa_t" << endl;
  }
  long double end_time= wclock();
  cout << "Ref parsed in " << end_time - start_time << " (s)" << endl;
  cout << endl;
  return n_factors;
}

template <typename sa_t>
size_t parse_ref(DictionaryFa<sa_t> &d,
                 char * reference_filename,
                 Writer &w,
                 int max_memory_MB) {
  cout << "Parsing the ref..." << endl;
  long double start_time= wclock();
  size_t n_factors; 
  //long max_block_size = (atoi(argv[2]) * (1L << 20)) / 29L;
  //size_t EMLZ_limit_MB = (29L*(INT32_MAX/2))/(1L<<20);
  //size_t EMLZ_mem_MB = std::min(EMLZ_limit_MB, (size_t)max_memory_MB);
  if (sizeof(sa_t) <= 4) {
		int LZ_mem_MB = (int)max_memory_MB;
		if (( LZ_mem_MB << 20) <= 0) {
			LZ_mem_MB = (INT32_MAX)>>20;
			cerr << "Max mem is too much for LZ Scan, adjusting to: " << LZ_mem_MB;
		}
    n_factors = lz_parse_ref(d.d, d.n, w, LZ_mem_MB);
  } else if (sizeof(sa_t) == 8) {
    bool enough_memory = true;  // TODO propper calculation.
    if (enough_memory) {
      cout << "USING KKP" << endl;
      cout << "for fastas, EM parsing of the reference is not implemented" << endl;
      n_factors = kkp_parse_ref(d.d, d.sa, d.n, w);
    } else {
      cerr << "for fastas, EM parsing of the reference is not implemented" << endl;
      exit(33);
      n_factors = em_parse_ref(reference_filename, w, max_memory_MB);
    }
  } else {
    cout << "Unfamiliar sa_t" << endl;
  }
  long double end_time= wclock();
  cout << "Ref parsed in " << end_time - start_time << " (s)" << endl;
  cout << endl;
  return n_factors;
}

}
#endif /* PARSEREF_H*/
