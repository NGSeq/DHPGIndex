#ifndef KERNELMANAGERBWA_H_
#define KERNELMANAGERBWA_H_

#include "utils.h"
#include "KernelManager.h"
#include <vector>
#include <sdsl/rmq_support.hpp>
#include <sdsl/util.hpp>
#include <sdsl/util.hpp>
#include <sdsl/io.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <sdsl/enc_vector.hpp>
#include <sdsl/coder_elias_delta.hpp>

using sdsl::int_vector;
using sdsl::util::clear;
using sdsl::rmq_succinct_sct;
using sdsl::enc_vector;
using sdsl::coder::elias_delta;
using sdsl::store_to_file;
using sdsl::load_from_file;
using sdsl::size_in_bytes;
using sdsl::csa_wt;
using sdsl::wt_huff;
using sdsl::rrr_vector;
using sdsl::construct;

class KernelManagerBWA : public KernelManager {
 public:
  // Index construction:
  KernelManagerBWA();
  KernelManagerBWA(uchar * text,
                   size_t text_len,
                   char * _kernel_text_filename,
                   int _verbose);
  void CreateKernelTextFile(uchar * kernel_text, size_t kernel_text_len);
  void DeleteKernelTextFile();
  void ComputeSize();
  virtual ~KernelManagerBWA();
  void SetFileNames(char * _prefix);
  void Load(char * _prefix, int n_threads, int verbose);
  void Save() const;
  size_t ComputeLength(); 


  // Queries and accessors:
  vector<Occurrence> LocateOccsFQ(char * query_filename,
                                  char * mates_filename,
                                  bool retrieve_all,
                                  bool single_file_paired,
                                  vector<string> kernel_options) const;
  vector<Occurrence> LocateOccs(string query) const;
  vector<string> ExtractSequences(uint64_t position) const;

    uint GetSizeBytes() const;
  void DetailedSpaceUssage() const;
  /*
  size_t GetLength() const {
    return kernel_text_len;
  }
  */
  static vector<Occurrence> SamOccurrences(const char * sam_filename);

 private:
  // Variables:
  int verbose;
  int n_threads;
  uint my_size_in_bytes;
  size_t header_len;
  //size_t kernel_text_len;

  //char kernel_text_filename[200];
  string kernel_text_filename;
  // Own files:
  //char kernel_index_filename[200];
  string kernel_index_filename;
};

#endif /* KERNELMANAGERBWA_H__*/
