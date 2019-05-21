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

#include "./KernelManagerFMI.h"
#include <sdsl/rmq_support.hpp>
#include <sdsl/util.hpp>
#include <tuple>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include "./utils.h"

KernelManagerFMI::KernelManagerFMI() {
    cerr << "++++++++++++++++++++++++++++" << endl;
    cerr << "Building FMI Kernel Manager " << endl;
    cerr << "++++++++++++++++++++++++++++" << endl;
}


KernelManagerFMI::KernelManagerFMI(uchar * text,
                                   size_t len,
                                   char * _kernel_text_filename,
                                   int _verbose) {
  verbose = _verbose;
  if (verbose >=2) {
    cerr << "++++++++++++++++++++++++++++" << endl;
    cerr << "Building FMI Kernel Manager " << endl;
    cerr << "++++++++++++++++++++++++++++" << endl;
  }
  SetFileNames(_kernel_text_filename);
  CreateKernelTextFile(text, len);
	delete [] text;
  my_size_in_bytes = 0;
  construct(index, kernel_text_filename.c_str(), 1);
  my_size_in_bytes += sdsl::size_in_bytes(index);
  kernel_text_len = index.size() - 1;
  Utils::DeleteTmpFile(kernel_text_filename);
  ComputeSize();
}

void KernelManagerFMI::CreateKernelTextFile(uchar * _kernel_text, size_t _kernel_text_len) {
  FILE * fp = Utils::OpenWriteOrDie(kernel_text_filename.c_str());
  if (_kernel_text_len != fwrite(_kernel_text, 1, _kernel_text_len, fp)) {
    cerr << "Error writing the kernel to a file" << endl;
    exit(1);
  }
  fclose(fp);
}

// TODO: this is dummy. Maybe using KernelManager as a template parameter to HI would fix that.
string  KernelManagerFMI::LocateOccsFQ(char * query_filename,
                                       char * mates_filename,
                                       bool retrieve_all,
                                       bool single_file_paired,
                                       vector<string> kernel_options) const {
  cerr << "We will not query file "<< string(query_filename) <<endl;
  cerr << "We will not query file "<< string(mates_filename) <<endl;
  cerr << "We ignore flags: " << retrieve_all << " and " << single_file_paired << "and kernel options of size" << kernel_options.size() << endl;
  cerr << "FMI Kernel Manager cannot handle FQs yet." << endl;
  cerr << "You may want to try with BWA Kernel Manager." << endl;
  exit(EXIT_FAILURE);
}

vector<Occurrence>  KernelManagerFMI::LocateOccs(string query) const {
  sdsl::int_vector<64u> locations = locate(index, query.begin(), query.end());
  vector<Occurrence> ans;
  ans.reserve(locations.size());
  for (size_t i = 0; i < locations.size(); i++) {
    ans.push_back(Occurrence(locations[i], query.size()));
  }
  return ans;
}

// ***********************************
// ACCESSORS
// ***********************************
void KernelManagerFMI::DetailedSpaceUssage() const {
  /*
  cerr << " RMQ             :" << size_in_bytes(new_rmq) << endl;
  cerr << " Sparse sample X :" << sparser_sample_X.size()*sizeof(uint64_t) << endl;
  cerr << " X               :" << size_in_bytes(new_X) << endl;
  cerr << " Ptr             :" << size_in_bytes(new_ptr) << endl;
  cerr << " Limits          :" << size_in_bytes(encoded_limits) << endl;
  cerr << " IsTerminalFlags :" << size_in_bytes(is_terminal) << endl;
  */
}

// ***********************************
// QUERIES
// ***********************************
void KernelManagerFMI::ComputeSize() {
  uint count = 0;
  count+=size_in_bytes(index);
  ASSERT(my_size_in_bytes== 0 || my_size_in_bytes == count);
  my_size_in_bytes = count;
}

void KernelManagerFMI::SetFileNames(char * _kernel_text_filename) {
  kernel_text_filename.assign(_kernel_text_filename);
  kernel_index_filename = kernel_text_filename + ".MAN.kernel_index";
}

void KernelManagerFMI::Save() const {
  store_to_file(index, kernel_index_filename);
}

void KernelManagerFMI::Load(char * _prefix, int _n_threads, int _verbose) {
  this->verbose = _verbose;
  this->n_threads = _n_threads;
  kernel_index_filename.assign(_prefix);
  SetFileNames(_prefix);
  load_from_file(index, kernel_index_filename);
  kernel_text_len = index.size() - 1;

  my_size_in_bytes = 0;
  ComputeSize();
}

KernelManagerFMI::~KernelManagerFMI() {
}
