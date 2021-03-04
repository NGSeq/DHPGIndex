// Copyright Daniel Valenzuela
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
                                           int threads,
                                           char * _kernel_text_filename,
                                           int _verbose) {
  verbose = _verbose;
  if (verbose >=2) {
    cout << "++++++++++++++++++++++++++++" << endl;
    cout << "Building BowTie2 Kernel Manager " << endl;
    cout << "++++++++++++++++++++++++++++" << endl;
  }
  SetFileNames(_kernel_text_filename);
  CreateKernelTextFile(input_kernel_text, input_len);
  my_size_in_bytes = 0;
  size_t kernel_text_len = this->ComputeLength();
  ASSERT(kernel_text_len = input_len);
#ifndef PROJECT_ROOT
  cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(-1);
#else
  //string bt2_command_index = string(PROJECT_ROOT);

  string bt2_command_index = "/opt/bowtie2/";
  bt2_command_index += "bowtie2-build --threads "+std::to_string(threads);
  bt2_command_index += " " + kernel_text_filename;
  bt2_command_index += " " + kernel_text_filename;
  bt2_command_index += " > " + kernel_text_filename + ".log 2>&1";

  if (verbose > 1) {
    cout << "-------------------------------------" << endl;
    cout << "To index the Kenrel, We wil call: " << endl << bt2_command_index << endl << endl;
    cout << "-------------------------------------" << endl;
  }
  if (system(bt2_command_index.c_str())) {
    cout << "Command failed. " << endl;
    exit(-1);
  }
#endif

  //Utils::DeleteTmpFile(kernel_text_filename);
  ComputeSize();
}

KernelManagerBowTie2::KernelManagerBowTie2(
                                           char * _kernel_text_filename,
                                           int threads,
                                           int _verbose) {
    verbose = _verbose;
    if (verbose >=2) {
        cout << "++++++++++++++++++++++++++++" << endl;
        cout << "Building BowTie2 Kernel Manager " << endl;
        cout << "++++++++++++++++++++++++++++" << endl;
    }
    SetFileNames(_kernel_text_filename);
    my_size_in_bytes = 0;
    size_t kernel_text_len = this->ComputeLength();

#ifndef PROJECT_ROOT
    cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
    exit(-1);
#else
    //string bt2_command_index = string(PROJECT_ROOT);
    cout << std::to_string(n_threads) << endl;
  std::string bt2_command_index = "/opt/bowtie2/";
  bt2_command_index += "bowtie2-build --threads "+std::to_string(threads);
  bt2_command_index += " " + kernel_text_filename;
  bt2_command_index += " " + kernel_text_filename;
  bt2_command_index += " > " + kernel_text_filename + ".log 2>&1";

  if (verbose > 1) {
    cout << "-------------------------------------" << endl;
    cout << "To index the Kenrel, We wil call: " << endl << bt2_command_index << endl << endl;
    cout << "-------------------------------------" << endl;
  }
  if (system(bt2_command_index.c_str())) {
    cout << "Command failed. " << endl;
    exit(-1);
  }
#endif

    //Utils::DeleteTmpFile(kernel_text_filename);
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

vector<Occurrence> KernelManagerBowTie2::SamOccurrences(const char * sam_filename) {
  vector<Occurrence> ans;
  std::ifstream ifile;
  ifile.open(sam_filename);
  if (!ifile.good() || !ifile.is_open()) {
    cerr << "Error loading patterns from '" << sam_filename << "'" << endl;
    exit(EXIT_FAILURE);
  }
  string line;
  string curr_message;
  while (getline(ifile, line)) {
    if (line[0] == '@') continue;  // TODO: Do something with the header ?
    ans.push_back(Occurrence(line));
  }
  return ans;
}

vector<Occurrence>  KernelManagerBowTie2::LocateOccsFQ(char * query_filename,
                                                       char * mates_filename,
                                                       bool retrieve_all,
                                                       bool single_file_paired,
                                                       vector<string> kernel_options) const {
  sdsl::int_vector<64u> locations;
  vector<Occurrence> ans;

#ifndef PROJECT_ROOT
  cerr << "PROJECT ROOT NOT DEFINED, DID YOU MODIFY THE MAKEFILE ?" << endl;
  exit(-1);
#else
  std::string basefn = kernel_text_filename.substr(kernel_text_filename.find_last_of("/\\") + 1);
  string  tmp_out_filename = "/mnt/tmp/aligned_to_kernel"+basefn+".sam";  // TODO: a better name ?
  string bt2_command_align;
  string all_flags="";
  /*if (retrieve_all) {
    all_flags.assign(" -a");  // bowties uses same flag as bwa. It's ok.
  } else {
    all_flags.assign("");
  }*/
  all_flags += " -p"+std::to_string(n_threads);
  //all_flags += " --very-sensitive"; 
  if (single_file_paired) {
    cerr << "Single file with paired end read not supported by BowTie2 Kernel" << endl;
    exit(2);
  }
  /*for (size_t i = 0; i < kernel_options.size(); i++) {
    all_flags += " "+kernel_options[i];
  }*/
  for (size_t i = 0; i < kernel_options.size(); i++) {
    all_flags += " "+kernel_options[i];
  }
  //bt2_command_align.assign(PROJECT_ROOT);
  bt2_command_align = "bowtie2 " + all_flags;
  bt2_command_align += " -x " + kernel_text_filename;  // we have used kernel_text_filename as basename for the index, should be ok.
  if (mates_filename == NULL) {
    bt2_command_align += " -U "+ string(query_filename);
  } else {
    bt2_command_align += " -1 "+ string(query_filename);
    bt2_command_align += " -2 "+ string(mates_filename);
    //bt2_command_align += " -1 "+ string(query_filename);
    //bt2_command_align += " -2 "+ string(mates_filename);
  }
  bt2_command_align += " -S "+ tmp_out_filename;
  bt2_command_align+= " 2> " + tmp_out_filename + ".log.stderr";
  if (verbose > 1) {
    cout << "-------------------------------------" << endl;
    cout << "To query the Kernel, we wil call: " << endl;
    cout << bt2_command_align << endl;
    cout << "-------------------------------------" << endl;
  }
  if (system(bt2_command_align.c_str())) {
    cerr << "Command failed. " << endl;
    exit(EXIT_FAILURE);
  }

  ans = KernelManagerBowTie2::SamOccurrences(tmp_out_filename.c_str());
  if (verbose) {
    cout << ans.size() << " SAM records w/r/to kernel" << endl;
  }
  return ans;
#endif
}

vector<Occurrence>  KernelManagerBowTie2::LocateOccs(string query) const {
  cerr << "KernelManagerBowTie2 is not ready to do this " << endl;
  cerr << "We will ignore query: " << query << " and abort..." << endl;
  exit(EXIT_FAILURE);
}


vector<string>  KernelManagerBowTie2::ExtractSequences(uint64_t position, uint64_t range) const {
    const std::basic_string<char> &seqs = extract(index, position-range, position+range);
    vector<string> ans;
    ans.reserve(seqs.size());
    for (size_t i = 0; i < seqs.size(); i++) {
        ans.push_back(seqs);
    }
    return ans;
}

// ***********************************
// ACCESSORS
// ***********************************
uint KernelManagerBowTie2::GetSizeBytes() const {
  return my_size_in_bytes;
}

void KernelManagerBowTie2::DetailedSpaceUssage() const {
  /*
     cout << " RMQ             :" << size_in_bytes(new_rmq) << endl;
     cout << " Sparse sample X :" << sparser_sample_X.size()*sizeof(uint64_t) << endl;
     cout << " X               :" << size_in_bytes(new_X) << endl;
     cout << " Ptr             :" << size_in_bytes(new_ptr) << endl;
     cout << " Limits          :" << size_in_bytes(encoded_limits) << endl;
     cout << " IsTerminalFlags :" << size_in_bytes(is_terminal) << endl;
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
  kernel_index_filename = kernel_text_filename + ".MAN.kernel_index";
}

void KernelManagerBowTie2::Save() const {
}

void KernelManagerBowTie2::Load(char * _prefix, int _n_threads, int _verbose) {
  this->verbose = _verbose;
  this->n_threads = _n_threads;
  kernel_index_filename.assign(_prefix);
  SetFileNames(_prefix);

  this->header_len = 15;  // TODO: This is the heacder len of the internal file for the kernel string.
  // look for other occurrences of header_len.
  // It would be  possible to read the first line instead...
  //kernel_text_len = this->ComputeLength();

  my_size_in_bytes = 0;
  ComputeSize();
}
size_t KernelManagerBowTie2::ComputeLength() {
  size_t plain_length = Utils::GetLength(kernel_text_filename.c_str());
  return plain_length - header_len;
}

KernelManagerBowTie2::~KernelManagerBowTie2() {
}
