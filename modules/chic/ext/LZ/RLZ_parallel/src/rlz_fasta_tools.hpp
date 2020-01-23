#ifndef RLZFASTATOOLS_H
#define RLZFASTATOOLS_H
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

#include "dictionary_fasta.h"
#include "fastametadata.hpp"
#include "hdfsmetadata.hpp"
#include "common.h"
#include "reference_parser.hpp"

using std::pair;
using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::pair;
using std::string;
using FMD::FastaMetaData;
using FMD::HDFSMetaData;

namespace RLZFasta{
  size_t flush_phrases(vector<vector<Factor>> factor_lists, Writer &w) {
    cout << "Flushing blocl phrases..." << endl;
    long double start_time= wclock();
    size_t n_factors = 0;
    w.flush();
    for (size_t t = 0; t < factor_lists.size(); t++) {
      n_factors += factor_lists[t].size(); 

      size_t data_len = factor_lists[t].size();
      if (data_len != fwrite(&(factor_lists[t][0]),
                             sizeof(Factor),
                             data_len, 
                             w.fp)) {
        std::cerr << "Error flushing phrases " << endl;
      }
    }
    long double end_time= wclock();
    cout << "Phrases flushed in " << end_time - start_time << " (s)" << endl;
    return n_factors;
  }

  void chunk_print_info(size_t block_id,
                        size_t chunk_id,
                        size_t starting_pos,
                        size_t length,
                        long double start_time,
                        long double end_time,
                        size_t n_phrases,
                        string message) {
    long double elapsed_seconds = end_time - start_time;
    std::ostringstream oss;
    oss << "log_block_" <<block_id << "_chunk_" << chunk_id << ".log";

    std::ofstream myfile;
    myfile.open (oss.str());

    auto cpu_id = sched_getcpu();
    myfile << "-----------------------------------------------" << endl;
    myfile << "chunk_id   :" << chunk_id << endl; 
    myfile << "cpu id     : " << cpu_id << endl; 
    myfile << "pos range  : " << starting_pos << "-" << starting_pos + length - 1 << endl;
    myfile << "length     : " <<  length << endl;
    myfile << "N phrases  : " <<  n_phrases<< endl;
    myfile << "T(s) range : " << start_time << "-" << end_time << endl;
    myfile << "Time(s)    : " << elapsed_seconds << endl;
    myfile << "Message    : " << message << endl;
    myfile << "-----------------------------------------------" << endl;
  }

  template <typename sa_t>
    void chunk_parse(uint8_t* buffer,
                     size_t length,
                     DictionaryFa<sa_t> &d,
                     vector<Factor> & ans,
                     FastaMetaData * metadata,
                     size_t abs_starting_pos) {
      size_t i = 0;
      size_t abs_pos = abs_starting_pos;
      while (i < length) {
        size_t skipped = 0;
        Factor factor = d.at(buffer + i, length - i, &skipped, metadata, abs_pos);
        if (skipped != length - i ) {
          ans.push_back(factor);
        } else {
          // skipped == length - i is the case when the parser was assigned a (last) 
          // part that was entirely header
          assert(factor.len == 0);
        }
        i += (factor.len > 0) ? factor.len : 1;
        i += skipped;
        abs_pos += (factor.len > 0) ? factor.len : 1;
        abs_pos += skipped;
      }
      return;
    }

  void read_block(uint8_t *buffer, size_t buffer_len, size_t block_id, FILE * fp) {
    cout << "Reading block " << block_id << "..." << endl;
    long double start_time= wclock();
    if (buffer_len != fread(buffer, 1, buffer_len, fp)) {
      printf("ERR");
      exit(-1);
    }
    long double end_time= wclock();
    cout << "Block read in " << end_time - start_time << " (s)" << endl;
    cout << endl;
  }

  template <typename sa_t>
    size_t process_block(char * input_filename,
                         uint8_t *buffer,
                         size_t buffer_len,
                         size_t block_offset,
                         size_t block_id,
                         FILE * fp,
                         size_t n_partitions,
                         DictionaryFa<sa_t>& d,
                         Writer& w) {
      read_block(buffer, buffer_len, block_id, fp);
      vector<vector<Factor>> factor_lists(n_partitions);
      cout << "Starting parallel parsing in block " << block_id << "..." << endl;
      cout << endl;
      long double start_time= wclock();
#pragma omp parallel for
      for (size_t t = 0; t < n_partitions; t++) {
        // MAYBE encapuslate all this block...
        FastaMetaData *metadata;
        metadata = new FastaMetaData();
        metadata->Load(string(input_filename)+".metadata");
        //metadata->Test();
        long double chunk_start_time= wclock();
        size_t starting_pos = t*(buffer_len/n_partitions);
        size_t length = (t != n_partitions - 1) ? (buffer_len/n_partitions) : buffer_len - starting_pos;
        size_t abs_starting_pos = starting_pos + block_offset;

        assert(d.n <= block_offset + starting_pos);

        chunk_parse(buffer+starting_pos, length, d, factor_lists[t], metadata, abs_starting_pos); 
        long double chunk_end_time = wclock();
        chunk_print_info(block_id, t, abs_starting_pos, length, chunk_start_time, chunk_end_time, factor_lists[t].size(), "");
        delete(metadata);
      }
      long double end_time= wclock();
      cout << "Parallel parse in " << end_time - start_time << " (s)" << endl;
      cout << endl;

      size_t n_factors = flush_phrases(factor_lists, w);
      return n_factors;
    }

  template <typename sa_t>
    size_t parse_in_external_memory(char * input_filename,
                                    char * reference_filename,
                                    size_t reference_len,
                                    size_t n_partitions,
                                    int max_memory_MB,
                                    Writer &w) {
      FastaMetaData *tmp;
      tmp = new FastaMetaData(string(input_filename));
      tmp->Save();
      //tmp->Test();
      delete(tmp);
      /*
         FastaMetaData *metadata;
         metadata = new FastaMetaData();
         metadata->Load(string(input_filename)+".metadata");
         metadata->Test();
         */
      // sample size can be set to MAX_INT to allow longer phrases and go to "normal" RLZ
      cout << "Ref len is: " << reference_len << endl;
      DictionaryFa<sa_t> d(reference_filename, reference_len, INT32_MAX);
      d.BuildSA();

      size_t n_factors = 0;
      n_factors = ReferenceParser::parse_ref<sa_t>(d, reference_filename, w, max_memory_MB);


      size_t text_len;
      FILE * fp = open_file(input_filename, &text_len);
      if (( (size_t)max_memory_MB << (size_t)20 ) < d.size_in_bytes()) {
        std::cerr << "Aborting because " << max_memory_MB << "MB is not enough." << endl;
        std::cerr << "The reference dictionary uses: " << d.size_in_bytes() << " bytes" << endl;
        exit(EXIT_FAILURE);
      }
      size_t block_len = ((size_t)max_memory_MB << (size_t)20) - d.size_in_bytes();
      if (block_len > text_len) {
        block_len = text_len;
      }
      cout << "We will read blocks of size: " << block_len << endl;
      size_t block_offset = d.in_file_n;  // no nedd of propper offset, we have metadata now. 
      //size_t block_offset = propper_offset(reference_filename, d.in_file_n);  
      // this indicates the cutoff point in the file, regardless of
      //underlying sequence position.
      fseek(fp, block_offset, SEEK_SET);
      size_t block_id = 0;
      uint8_t *buffer = new uint8_t[block_len + 1];

      while (block_offset < text_len) {
        size_t buffer_len = (block_offset + block_len < text_len) ? block_len : text_len - block_offset;
        n_factors += process_block(input_filename,
                                   buffer,
                                   buffer_len,
                                   block_offset,
                                   block_id,
                                   fp,
                                   n_partitions,
                                   d,
                                   w);

        block_offset += buffer_len;
        block_id++;
      }
      delete [] buffer;
      fclose(fp);
      printf("Parsing is ready\n");
      fflush(stdout);
      return n_factors;
    }
}
#endif /* RLZFASTATOOLS_H*/
