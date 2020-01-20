// Copyright Daniel Valenzuela
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
    cout << "++++++++++++++++++++++++++++" << endl;
    cout << "Building FMI Kernel Manager " << endl;
    cout << "++++++++++++++++++++++++++++" << endl;
}


KernelManagerFMI::KernelManagerFMI(uchar * text,
                                   size_t len,
                                   char * _kernel_text_filename,
                                   int _verbose) {
  verbose = _verbose;
  if (verbose >=2) {
    cout << "++++++++++++++++++++++++++++" << endl;
    cout << "Building FMI Kernel Manager " << endl;
    cout << "++++++++++++++++++++++++++++" << endl;
  }
  SetFileNames(_kernel_text_filename);
  //TODO: Separate this
  CreateKernelTextFile(text, len);
  my_size_in_bytes = 0;
  construct(index, kernel_text_filename.c_str(), 1);
  my_size_in_bytes += sdsl::size_in_bytes(index);
  kernel_text_len = index.size() - 1;
  //Utils::DeleteTmpFile(kernel_text_filename);
  ComputeSize();
}

KernelManagerFMI::KernelManagerFMI(char * _kernel_text_filename,
                                   int _verbose) {
    verbose = _verbose;
    if (verbose >=2) {
        cout << "++++++++++++++++++++++++++++" << endl;
        cout << "Building FMI Kernel Manager " << endl;
        cout << "++++++++++++++++++++++++++++" << endl;
    }
    SetFileNames(_kernel_text_filename);
    //TODO: Separate this

    my_size_in_bytes = 0;
    construct(index, kernel_text_filename.c_str(), 1);
    my_size_in_bytes += sdsl::size_in_bytes(index);
    kernel_text_len = index.size() - 1;
    //Utils::DeleteTmpFile(kernel_text_filename);
    ComputeSize();
}

void KernelManagerFMI::CreateKernelTextFile(uchar * _kernel_text, size_t _kernel_text_len) {
  FILE * fp = Utils::OpenWriteOrDie(kernel_text_filename.c_str());
  if (_kernel_text_len != fwrite(_kernel_text, 1, _kernel_text_len, fp)) {
    cout << "Error writing the kernel to a file" << endl;
    exit(1);
  }
  fclose(fp);
}

// TODO: this is dummy. Maybe using KernelManager as a template parameter to HI would fix that.
vector<Occurrence>  KernelManagerFMI::LocateOccsFQ(char * query_filename,
                                                   char * mates_filename,
                                                   bool retrieve_all,
                                                   bool single_file_paired,
                                                   vector<string> kernel_options) const {
  cerr << "We will not query file "<< string(query_filename) <<endl;
  cerr << "We will not query file "<< string(mates_filename) <<endl;
  cerr << "We ignore flags: " << retrieve_all << " and " << single_file_paired << endl;
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

vector<string>  KernelManagerFMI::ExtractSequences(uint64_t position,uint64_t range) const {
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
uint KernelManagerFMI::GetSizeBytes() const {
  return my_size_in_bytes;
}

void KernelManagerFMI::DetailedSpaceUssage() const {
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
