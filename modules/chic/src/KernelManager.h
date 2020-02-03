#ifndef KERNELMANAGER_H_
#define KERNELMANAGER_H_

#include "utils.h"
#include "occurrence.h"
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

class KernelManager {
 public:
  // Index construction:
  KernelManager() {
  };
  KernelManager(uchar * text,
                size_t len,
                int n_threads,
                char * _kernel_text_filename,
                int _verbose);
    KernelManager(char * _kernel_text_filename,
                  int n_threads,
                  int _verbose);
  void CreateKernelTextFile(uchar * kernel_text, size_t kernel_text_len);
  virtual void ComputeSize() = 0;
  virtual ~KernelManager() {};
  virtual void SetFileNames(char * _prefix) = 0;
  virtual void Load(char * _prefix, int n_threads, int verbose) = 0;
  virtual void Save() const = 0;


  // Queries and accessors:
  virtual vector<Occurrence> LocateOccs(string query) const = 0;
  virtual vector<string> ExtractSequences(uint64_t position, uint64_t range) const = 0;

    virtual vector<Occurrence> LocateOccsFQ(char * query_filename,
                                          char * mates_filename,
                                          bool retrieve_all,
                                          bool single_file_paired,
                                          vector<string> kernel_options) const = 0;
  virtual uint GetSizeBytes() const = 0;
  virtual void DetailedSpaceUssage() const = 0;
  //virtual size_t GetLength() const = 0;

};

#endif /* KERNELMANAGER_H__*/
