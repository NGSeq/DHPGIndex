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

#include "./KernelManagerBowTie2.h"
#include <sdsl/rmq_support.hpp>
#include <sdsl/util.hpp>
#include <tuple>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include "./utils.h"

KernelManagerBowTie2::KernelManagerBowTie2() {
}

KernelManagerBowTie2::KernelManagerBowTie2(uchar * input_kernel_text,
                                           size_t input_len,
                                           char * _kernel_text_filename,
                                           int _n_threads,
                                           int _max_memory_MB,
                                           int _verbose) {
  verbose = _verbose;
  n_threads = _n_threads;
	max_memory_MB = _max_memory_MB;
  if (verbose >=2) {
    cerr << "++++++++++++++++++++++++++++" << endl;
    cerr << "Building BowTie2 Kernel Manager " << endl;
    cerr << "++++++++++++++++++++++++++++" << endl;
  }
  SetFileNames(_kernel_text_filename);
  CreateKernelTextFile(input_kernel_text, input_len);
	delete [] input_kernel_text;
  my_size_in_bytes = 0;
  size_t kernel_text_len = this->ComputeLength();
  ASSERT(kernel_text_len = input_len);
  string bt2_command_index = string(PROJECT_ROOT);
  //bt2_command_index += "/ext/BOWTIE2/bowtie2-2.3.0/bowtie2-build";
  bt2_command_index += "/opt/bowtie2/bowtie2-build";
  bt2_command_index += " --threads="+std::to_string(n_threads);
	if (input_len > (2ul<<30) && _max_memory_MB < 32000) {
		// if Bowtie "test ahead" was not killed this should not be necessary.
		cerr << "Bowtie build will use --packed option" << endl;
  	bt2_command_index += " --noauto --packed";
  	//bt2_command_index += " --noauto --packed --dcv 2048 --bmaxdivn 128";
	}
  bt2_command_index += " " + kernel_text_filename;
  bt2_command_index += " " + kernel_text_filename;
	if (verbose >= 3) {
  	bt2_command_index += " 1>&2";
	} else {
  	bt2_command_index += " > " + kernel_text_filename + ".log 2>&1";
	}

  if (verbose >= 2) {
    cerr << "-------------------------------------" << endl;
    cerr << "To index the Kenrel, We wil call: " << endl << bt2_command_index << endl << endl;
    cerr << "-------------------------------------" << endl;
  }

	int return_value = system(bt2_command_index.c_str());
	if (return_value) {
    cerr << "Command failed, returned value: " << return_value << endl;
	  cerr << "Errno: " << std::strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }
#endif

  Utils::DeleteTmpFile(kernel_text_filename);
  ComputeSize();
}

void KernelManagerBowTie2::CreateKernelTextFile(uchar * _kernel_text, size_t _kernel_text_len) {
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

string KernelManagerBowTie2::LocateOccsFQ(char * query_filename,
                                                       char * mates_filename,
                                                       bool retrieve_all,
                                                       bool single_file_paired,
                                                       vector<string> kernel_options) const {
  string  tmp_out_filename = "./.reads_aligned_to_kernel.sam";  // TODO: a better, non-unique name (hash-value based)
  string bt2_command_align;
  string all_flags;
  if (retrieve_all) {
    all_flags.assign(" -a");  // bowties uses same flag as bwa. It's ok.
  } else {
    all_flags.assign("");
  }
  all_flags += " --threads="+std::to_string(n_threads);
  all_flags += " --score-min C,10,0";
  if (single_file_paired) {
    cerr << "Single file with paired end read not supported by BowTie2 Kernel" << endl;
    exit(2);
  }
  for (size_t i = 0; i < kernel_options.size(); i++) {
    all_flags += " "+kernel_options[i];
  }
  bt2_command_align.assign("");
  //bt2_command_align += "/ext/BOWTIE2/bowtie2-2.3.0/bowtie2 " + all_flags;
  bt2_command_align += "/opt/bowtie2/bowtie2 " + all_flags;
  bt2_command_align += " -x " + kernel_text_filename;  // we have used kernel_text_filename as basename for the index, should be ok.
  if (mates_filename == NULL) {
    bt2_command_align += " -U "+ string(query_filename);
  } else {
		// As the reads are aligned to the Kernel String, the distances are unrelated with actual distances.
		// so we don't want the aligner to use any knowlede about the "parity" of the reads.
    // bt2_command_align += " -1 "+ string(query_filename);
    // bt2_command_align += " -2 "+ string(mates_filename);
    // We wanted the reads to be treated like twp independen single-end, as follows:
    bt2_command_align += " -U "+ string(query_filename);
    bt2_command_align += " -U "+ string(mates_filename);
  }
  bt2_command_align += " -S "+ tmp_out_filename;
  if (verbose >= 3) {
  	bt2_command_align+= " 1>&2";
	} else {
  	bt2_command_align+= " 2> " + tmp_out_filename + ".log.stderr";
	}
	if (verbose >= 2) {
    cerr << "-------------------------------------" << endl;
    cerr << "To query the Kernel, we wil call: " << endl;
    cerr << bt2_command_align << endl;
    cerr << "-------------------------------------" << endl;
  }
  if (system(bt2_command_align.c_str())) {
    cerr << "Command failed. " << endl;
    exit(EXIT_FAILURE);
  }
  return tmp_out_filename;
#endif
}

vector<Occurrence>  KernelManagerBowTie2::LocateOccs(string query) const {
  cerr << "KernelManagerBowTie2 is not ready to do this " << endl;
  cerr << "We will ignore query: " << query << " and abort..." << endl;
  exit(EXIT_FAILURE);
}

// ***********************************
// ACCESSORS
// ***********************************
void KernelManagerBowTie2::DetailedSpaceUssage() const {
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
void KernelManagerBowTie2::ComputeSize() {
  uint count = 0;
  ASSERT(my_size_in_bytes== 0 || my_size_in_bytes == count);
  my_size_in_bytes = count;
}

void KernelManagerBowTie2::SetFileNames(char * _kernel_text_filename) {
  kernel_text_filename.assign(_kernel_text_filename);
  // UNUSED, Bowtie uses its own set of files.
	// kernel_index_filename = kernel_text_filename + ".MAN.kernel_index";
}

void KernelManagerBowTie2::Save() const {
}

void KernelManagerBowTie2::Load(char * _prefix, int _n_threads, int _verbose) {
  this->verbose = _verbose;
  this->n_threads = _n_threads;
  // UNUSED, Bowtie uses its own set of files.
  // kernel_index_filename.assign(_prefix);
  SetFileNames(_prefix);

  this->header_len = 15;  // TODO: This is the heacder len of the internal file for the kernel string.
  // look for other occurrences of header_len.
  // It would be  possible to read the first line instead...
  // kernel_text_len = this->ComputeLength();

  my_size_in_bytes = 0;
  ComputeSize();
}
size_t KernelManagerBowTie2::ComputeLength() {
  size_t plain_length = Utils::GetLength(kernel_text_filename.c_str());
  return plain_length - header_len;
}

KernelManagerBowTie2::~KernelManagerBowTie2() {
}
