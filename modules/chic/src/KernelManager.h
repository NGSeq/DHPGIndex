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
  void CreateKernelTextFile(uchar * kernel_text, size_t kernel_text_len);
  virtual void ComputeSize() = 0;
  virtual ~KernelManager() {};
  virtual void SetFileNames(char * _prefix) = 0;
  virtual void Load(char * _prefix, int n_threads, int verbose) = 0;
  virtual void Save() const = 0;


  // Queries and accessors:
  virtual vector<Occurrence> LocateOccs(string query) const = 0;
  virtual string  LocateOccsFQ(char * query_filename,
                                          char * mates_filename,
                                          bool retrieve_all,
                                          bool single_file_paired,
                                          vector<string> kernel_options) const = 0;
  virtual void DetailedSpaceUssage() const = 0;
  //virtual size_t GetLength() const = 0;
  uint GetSizeBytes() const {
    return my_size_in_bytes;
  }

 protected:
  int verbose;
  int n_threads;
  int max_memory_MB;
  uint my_size_in_bytes;

};

#endif /* KERNELMANAGER_H__*/
