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

#ifndef KERNELMANAGERFMI_H_
#define KERNELMANAGERFMI_H_

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

class KernelManagerFMI : public KernelManager {
 public:
  // Index construction:
  KernelManagerFMI();
  KernelManagerFMI(uchar * text,
                   size_t text_len,char * _kernel_text_filename,
                   int _verbose);
  void CreateKernelTextFile(uchar * kernel_text, size_t kernel_text_len);
  void DeleteKernelTextFile();
  void ComputeSize();
  virtual ~KernelManagerFMI();
  void SetFileNames(char * _prefix);
  void Load(char * _prefix, int n_threads, int verbose);
  void Save() const;


  // Queries and accessors:
  string LocateOccsFQ(char * query_filename,
                      char * mates_filename,
                      bool retrieve_all,
                      bool single_file_paired,
                      vector<string> kernel_options) const;
  vector<Occurrence> LocateOccs(string query) const;
  void DetailedSpaceUssage() const;
  size_t GetLength() const {
    return kernel_text_len;
  }

 private:
  // Variables:
  csa_wt<wt_huff<rrr_vector<127> >, 512, 1024> index;
  size_t kernel_text_len;

  //char kernel_text_filename[200];
  string kernel_text_filename;
  // Own files:
  //char kernel_index_filename[200];
  string kernel_index_filename;
};

#endif /* KERNELMANAGERFMI_H__*/
