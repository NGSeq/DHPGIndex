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

#include "./KernelManagerBWA.h"
#include <sdsl/rmq_support.hpp>
#include <sdsl/util.hpp>
#include <tuple>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include "./utils.h"

KernelManagerBWA::KernelManagerBWA() {
}

KernelManagerBWA::KernelManagerBWA(char * _kernel_text_filename,
                                   int _verbose) {
    verbose = _verbose;
    if (verbose >=2) {
        cout << "++++++++++++++++++++++++++++" << endl;
        cout << "Building BWA Kernel Manager " << endl;
        cout << "++++++++++++++++++++++++++++" << endl;
    }
    SetFileNames(_kernel_text_filename);

    my_size_in_bytes = 0;
    size_t kernel_text_len = this->ComputeLength();
#ifndef PROJECT_ROOT
    cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
    exit(-1);
#else
    char * command_bwa_index= new char[1024];
  sprintf(command_bwa_index,
          "bwa index %s >%s.log_bwaindex 2>&1",
          kernel_text_filename.c_str(),
          kernel_text_filename.c_str());
  if (verbose > 1) {
  cout << "-------------------------------------" << endl;
  cout << "To index the Kenrel, We wil call:" << endl;
  cout <<  command_bwa_index << endl ;
  cout << "-------------------------------------" << endl;
  }
  if (system(command_bwa_index)) {
    cout << "Command failed. " << endl;
    exit(-1);
  }
  delete[] command_bwa_index;
#endif

    //Utils::DeleteTmpFile(kernel_text_filename);
    ComputeSize();
}

KernelManagerBWA::KernelManagerBWA(uchar * input_kernel_text,
                                   size_t input_len,
                                   char * _kernel_text_filename,
                                   int _verbose) {
  verbose = _verbose;
  if (verbose >=2) {
    cerr << "++++++++++++++++++++++++++++" << endl;
    cerr << "Building BWA Kernel Manager " << endl;
    cerr << "++++++++++++++++++++++++++++" << endl;
  }
  SetFileNames(_kernel_text_filename);
  CreateKernelTextFile(input_kernel_text, input_len);
	delete [] input_kernel_text;
  my_size_in_bytes = 0;
  size_t kernel_text_len = this->ComputeLength();
  ASSERT(kernel_text_len = input_len);
#ifndef PROJECT_ROOT
  cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(-1);
#else
  char * command_bwa_index= new char[1024];
  if (verbose >= 3) {
	  sprintf(command_bwa_index,
  	        "%sbwa index %s 1>&2",
    	      PROJECT_ROOT,
        	  kernel_text_filename.c_str());
	} else {
  	sprintf(command_bwa_index,
    	      "%sbwa index %s >%s.log_bwaindex 2>&1",
      	    PROJECT_ROOT,
        	  kernel_text_filename.c_str(),
          	kernel_text_filename.c_str());
	
	}

  if (verbose >= 2) {
  	cerr << "-------------------------------------" << endl;
  	cerr << "To index the Kenrel, We will call:" << endl;
  	cerr <<  command_bwa_index << endl;
  	cerr << "-------------------------------------" << endl;
  }
  if (system(command_bwa_index)) {
    cerr << "Command failed. " << endl;
    exit(-1);
  }
  delete[] command_bwa_index;
#endif

  Utils::DeleteTmpFile(kernel_text_filename);
  ComputeSize();
}

void KernelManagerBWA::CreateKernelTextFile(uchar * _kernel_text, size_t _kernel_text_len) {
  FILE * fp = Utils::OpenWriteOrDie(kernel_text_filename.c_str());
  string header = ">Dummy_header.\n";
  header_len = 15;
  if (header_len != fwrite(header.c_str(), 1, header_len, fp)) {
    cerr << "Error writing the kernel to a file" << endl;
    exit(1);
  }

  if (_kernel_text_len != fwrite(_kernel_text, 1, _kernel_text_len, fp)) {
    cerr << "Error writing the kernel to a file" << endl;
    exit(1);
  }
  fclose(fp);
}

string  KernelManagerBWA::LocateOccsFQ(char * query_filename,
                                                   char * mates_filename,
                                                   bool retrieve_all,
                                                   bool single_file_paired,
                                                   vector<string> kernel_options) const {
#ifndef PROJECT_ROOT
  cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(-1);
#else
  srand(time(0));
  string tmp_out_filename = "./.reads_aligned_to_kernel."  + std::to_string(std::rand()) +".sam"; 
  string command_bwa_mem;
  string all_flags;
  if (retrieve_all) {
    all_flags.assign(" -a");
  } else {
    all_flags.assign("");
  }
  if (single_file_paired) {
    all_flags += " -p";
  }
  all_flags += " -t"+std::to_string(n_threads);
  for (size_t i = 0; i < kernel_options.size(); i++) {
    all_flags += " "+kernel_options[i];
  }
  command_bwa_mem.assign(PROJECT_ROOT);
  command_bwa_mem += "bwa mem " + all_flags;
  command_bwa_mem += " " + kernel_text_filename;
  command_bwa_mem += " " + string(query_filename);
  if (mates_filename != NULL) {
    ASSERT(!single_file_paired);
    command_bwa_mem += " " + string(mates_filename);
  }
  command_bwa_mem += " > " + tmp_out_filename;
	if (verbose >= 3) {
  	command_bwa_mem += " 1>&2 ";
	} else {
  	command_bwa_mem += " 2> " + tmp_out_filename + ".log.stderr";
	}

  if (verbose >= 2) {
    cerr << "-------------------------------------" << endl;
    cerr << "To query the Kernel, we wil call: " << endl;
    cerr << command_bwa_mem << endl;
    cerr << "-------------------------------------" << endl;
  }
  if (system(command_bwa_mem.c_str())) {
    cerr << "Command failed. " << endl;
    exit(EXIT_FAILURE);
  }
  return tmp_out_filename;
#endif
}

vector<Occurrence>  KernelManagerBWA::LocateOccs(string query) const {
  cerr << "KernelManagerBWA is not ready to do this " << endl;
  cerr << "We will ignore query: " << query << " and abort..." << endl;
  exit(EXIT_FAILURE);
}

// ***********************************
// ACCESSORS
// ***********************************
void KernelManagerBWA::DetailedSpaceUssage() const {
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
void KernelManagerBWA::ComputeSize() {
  uint count = 0;
  ASSERT(my_size_in_bytes== 0 || my_size_in_bytes == count);
  my_size_in_bytes = count;
}

void KernelManagerBWA::SetFileNames(char * _kernel_text_filename) {
  kernel_text_filename.assign(_kernel_text_filename);
  kernel_index_filename = kernel_text_filename + ".MAN.kernel_index";
}

void KernelManagerBWA::Save() const {
}

void KernelManagerBWA::Load(char * _prefix, int _n_threads, int _verbose) {
  this->verbose = _verbose;
  this->n_threads = _n_threads;
  kernel_index_filename.assign(_prefix);
  SetFileNames(_prefix);

  this->header_len = 15;  // TODO: This is the heacder len of the internal file for the kernel string.
  // look for other occurrences of header_len.
  // It would be  possible to read the first line instead...
  // kernel_text_len = this->ComputeLength();

  my_size_in_bytes = 0;
  ComputeSize();
}
size_t KernelManagerBWA::ComputeLength() {
  size_t plain_length = Utils::GetLength(kernel_text_filename.c_str());
  return plain_length - header_len;
}

KernelManagerBWA::~KernelManagerBWA() {
}
