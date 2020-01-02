// Copyright Daniel Valenzuela
// Namespace to manage call to external LZ parsers.
//
//  We assume 64 bits per number in the phrases saved to disk.
//
#include "./LempelZivParser.h"
#include <vector>
#include <utility>
#include <iostream>
#include <string>
#include <algorithm>
#include "../ext/LZ/LZscan/algorithm/lzscan.h"

namespace LempelZivParser {
// TODO: I shouldn't try to "guess" the .lzparse file.
// Either I take it as a paremeter, or I build it.
void GetLZPhrases(vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr, HybridLZIndex * HY) {
  uchar * tmp_seq = HY->GetTmpSeq();
  size_t text_len = HY->GetTextLength();
  char * text_filename = HY->GetTextFileName();
  LZMethod lz_method = HY->GetLZMethod();
  int max_memory_MB = HY->GetMaxMemoryMB();
  int rlz_ref_len_MB = HY->GetRLZRefLength();
  int n_threads = HY->GetNThreads();
  string lz_parse_tmp_filename = string(HY->GetTextFileName()) + ".tmp.lzparse";
  switch (lz_method) {
    case LZMethod::IN_MEMORY:{
      cout << "IM" << endl;
      LempelZivParser::GetParseLZScan(tmp_seq, text_len, lz_phrases_ptr, max_memory_MB);
      LempelZivParser::SaveLZParse(lz_phrases_ptr, lz_parse_tmp_filename.c_str());
    }
    break;

    case LZMethod::EXTERNAL_MEMORY:{
      cout << "EM" << endl;
      LempelZivParser::GetParseEM(text_filename, lz_parse_tmp_filename.c_str(), lz_phrases_ptr, max_memory_MB);
    }
    break;

    case LZMethod::RLZ:{
      cout << "RLZ" << endl;
      if (rlz_ref_len_MB == 0) {
        // TODO: be smarter here.
        rlz_ref_len_MB = std::min((size_t)1000, text_len);  // 1 gig as the SA can use 32bits ... but a better computation should be used
        cout << "Warning: RLZ reference length was not specified. Using " << rlz_ref_len_MB << "MB" << endl;
      }
      LempelZivParser::GetParseRLZ(text_filename, lz_parse_tmp_filename.c_str(), lz_phrases_ptr, max_memory_MB, rlz_ref_len_MB, n_threads);
    }
    break;

    case LZMethod::INPUT:{
      cout << "INPUT LZ PARSE" << endl;
      FILE * lz_infile  = Utils::OpenReadOrDie(HY->GetInputLZFilename());  // TODO: seteralo en HY...
      LempelZivParser::LoadLZParse(lz_infile, lz_phrases_ptr);
      fclose(lz_infile);
    }
    break;

    default:
    {
      cerr << "I don't know this LZ Construction method" << endl;
      exit(-1);
    }

  }
}
void LoadLZParse(FILE * fp,
                 vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr) {
  // We assume 64 bits per number
  fseek(fp, 0, SEEK_END);
  size_t file_len = (size_t)ftell(fp);
  fseek(fp, 0, SEEK_SET);
  // beware this len is the numbers of bytes.
  file_len = file_len*sizeof(uchar)/sizeof(uint64_t);
  ASSERT(file_len % 2 == 0);
  uint64_t * buff = new uint64_t[file_len];
  if (file_len != fread(buff, sizeof(uint64_t), file_len, fp)) {
    cerr << stderr << "Error reading string from file" << endl;
    exit(EXIT_FAILURE);
  }
  size_t n_phrases = file_len/2;

  lz_phrases_ptr->reserve(n_phrases);
  for (size_t i = 0; i < n_phrases; i++) {
    uint64_t pos = buff[2*i];
    uint64_t len = buff[2*i+1];
    lz_phrases_ptr->push_back(pair<uint64_t, uint64_t>(pos, len));
  }
  delete [] buff;
}

void SaveLZParse(vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                 string lzparse_filename) {
  // Current standard: 64 bits per number
  // TODO: skip the buffer and save directly. 
  // Buffer was necessary when in memory we used 32 bits per len, now
  // what we keep in mem is same as in file, 64bits per each number.
  size_t n_phrases = lz_phrases_ptr->size();
  uint64_t * buff = new uint64_t[2*n_phrases];
  for (size_t i = 0; i < n_phrases; i++) {
    buff[2*i] = lz_phrases_ptr->at(i).first;
    buff[2*i + 1] = lz_phrases_ptr->at(i).second;
  }

  FILE * fp = Utils::OpenWriteOrDie(lzparse_filename.c_str());
  if (2*n_phrases != fwrite(buff, sizeof(uint64_t), 2*n_phrases, fp)) {
    cerr << "Error writing the LZ parse" << endl;
    exit(1);
  }

  fclose(fp);
  delete [] buff;
}

void GetParseEM(char * filename, string lzparse_filename,
                vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr, int max_memory_MB) {
#ifndef PROJECT_ROOT
  cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(EXIT_FAILURE);
#else
  string command_parse;
  command_parse.assign(PROJECT_ROOT);
  command_parse += "/ext/LZ/EM-LZscan-0.2/src/emlz_parser " + string(filename);
  command_parse += " --mem=" + std::to_string(max_memory_MB);
  command_parse += " --output=" + lzparse_filename;
  command_parse += " >" + lzparse_filename + ".log_emlzparse 2>&1";
  //if (verbose > 1) 
  cout << "-------------------------------------" << endl;
  cout << "To obtain LZ parse we will call: " << command_parse << endl << endl;
  cout << "-------------------------------------" << endl;
  //}
  if (system(command_parse.c_str())) {
    cerr << "Command failed. " << endl;
    exit(-1);
  }

  FILE *lz_infile = Utils::OpenReadOrDie(lzparse_filename.c_str());
  LempelZivParser::LoadLZParse(lz_infile, lz_phrases_ptr);
  fclose(lz_infile);
#endif
}

void GetParseRLZ(char * filename,
                 string lzparse_filename,
                 vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr,
                 int max_memory_MB,
                 int rlz_ref_len_MB,
                 int n_threads) {
  int n_chunks = n_threads;  // would it be better to have 2 or three times n_threads ?
#ifndef PROJECT_ROOT
  cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(EXIT_FAILURE);
#else
  string command_parse;
  command_parse.assign(PROJECT_ROOT);
  command_parse += "/ext/LZ/RLZ_parallel/src/rlz_parser.sh " + string(filename) + " " + lzparse_filename;
  command_parse += " " + std::to_string(rlz_ref_len_MB);
  command_parse += " " + std::to_string(n_chunks);
  command_parse += " " + std::to_string(n_threads);
  command_parse += " " + std::to_string(max_memory_MB);
  command_parse += " >" + lzparse_filename + ".log_RLZparse 2>&1";
  //if (verbose > 1) {
    cout << "-------------------------------------" << endl;
    cout << "To obtain LZ parse we will call: " << command_parse << endl << endl;
    cout << "-------------------------------------" << endl;
  //}
  if (system(command_parse.c_str())) {
    cerr << "Command failed. " << endl;
    exit(-1);
  }

  FILE *lz_infile = Utils::OpenReadOrDie(lzparse_filename.c_str());
  LempelZivParser::LoadLZParse(lz_infile, lz_phrases_ptr);
  fclose(lz_infile);
#endif
}

void GetParseLZScan(uchar *seq,
                    size_t seq_len,
                    vector<pair<uint64_t, uint64_t>> * lz_phrases_ptr, int max_memory_MB) {
  vector<pair<int, int>> tmp_phrases;
  // TODO: ASSERT that texts fits in int, otherwise EM should be used !
  size_t n_phrases = (uint)parse(seq, (int)seq_len, max_memory_MB << 20, &tmp_phrases);

  lz_phrases_ptr->reserve(n_phrases);
  for (size_t i = 0; i < n_phrases; i++) {
    uint64_t first  = (uint64_t)tmp_phrases.at(i).first;
    uint64_t second = (uint64_t)tmp_phrases.at(i).second;
    lz_phrases_ptr->push_back(pair<uint64_t, uint64_t>(first, second));
  }
  // TODO: maybe HI should print this info
  // cout << " N of phrases z = " << n_phrases;
  // cout << " = " << (float)n_phrases/(float)seq_len << "*n" << endl;
}
}  // namespace LempelZivParser
