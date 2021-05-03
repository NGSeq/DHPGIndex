/*  Copyright Daniel Valenzuela
 *  Scan and store metadata info of a fasta file so later 
 *  it can be navigated like a file that contains a 
 *  bare sequence.
 *  We require that all the data lines have the same length 
 *  OR that there are no breaklines and each genome sequence 
 *  is in one line
 * */
#ifndef METAFASTA_H 
#define METAFASTA_H 

#include <cassert>
#include <vector>
#include <string>
#include <fstream>
#include <limits.h>
//#include "./basic.h"

#define LEN_UNSET ((size_t)UINT_MAX)
#define LEN_UNLIMITED ((size_t)UINT_MAX-1)
//#define LEN_THRESHOLD ((size_t)1000)

using std::pair;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::string;
namespace FMD {
class FastaMetaData {
 public:
  FastaMetaData() {
  }
  FastaMetaData(string _fasta_filename, bool verbose=false) {
    cache_i = 0;
    fasta_filename.assign(_fasta_filename);
    meta_filename = fasta_filename + ".metadata";
    if (FileExists(meta_filename)) {
      if (verbose)
        cout << "Metadata file exists, will load it" << endl;
      Load(meta_filename);
      return;
    }
    if (verbose)
      cout << "metadata construction..." << endl;
    std::ifstream fs;
    fs.open(fasta_filename);
    if(fs.fail()) {
      std::cerr << "Error opening " << fasta_filename << endl;
      exit(EXIT_FAILURE);
    }
    assert(fs.good());

    size_t seq_i = 0;
    seq_names.clear();
    seq_lengths.clear();

    size_t byte_pos = 0;
    size_t tot_seq_pos = 0;
    size_t curr_seq_pos = 0;
    line_len = LEN_UNSET;
    size_t last_break_pos = 0;
    bool header = false;
    string curr_name;
    int breaks_in_dna = 0;
    while (!fs.eof()) {
      char curr_char = fs.get();

      if (curr_char == '>') {
        headers_sp.push_back(byte_pos);
        header = true;
        curr_name.assign("");
        if (tot_seq_pos != 0) {
          seq_lengths.push_back(curr_seq_pos);
          curr_seq_pos = 0;
        }
      }

      if (header) {
        if (curr_char == '\r' ||curr_char == '\n') {
          last_break_pos = byte_pos;
          headers_ep.push_back(byte_pos);
          header = false;
          curr_name.erase(0, 1);  // remove the '>' symbol
          seq_names.push_back(curr_name);
          seq_i++;
          breaks_in_dna = 0;
        } else {
          curr_name += curr_char;
        }
      } else {
        if (curr_char == '\r' || curr_char == '\n') {
          breaks_in_dna++;
          size_t curr_length = byte_pos - last_break_pos - 1;
          if (breaks_in_dna > 1) {
            //assert(line_len == curr_length || line_len == LEN_UNSET);  // fails in the last line...
            if (line_len == LEN_UNSET) {
              line_len = curr_length;
            } 
          }
          last_break_pos = byte_pos;
        } else {
          curr_seq_pos++;
          tot_seq_pos++;
        }
      }
      byte_pos++;
    }
    if (line_len == LEN_UNSET ) {
      line_len = LEN_UNLIMITED;
      //std::cerr << "We have a fasta without breaks within dna seqs" << endl;
    }
    n_seqs = seq_i;
    seq_lengths.push_back(curr_seq_pos-1);  // TODO: this is a difference from the RLZ version of this file.
    file_len = byte_pos;
    assert(n_seqs == seq_names.size());
    assert(n_seqs == seq_lengths.size());
    assert(n_seqs == headers_sp.size());
    assert(n_seqs == headers_ep.size());
    assert(headers_sp[0] == 0);
    BuildSeqLengthsSums();
    if (verbose)
      PrintInfo();
  }
  bool FileExists(string file) {
    FILE * fp = fopen(file.c_str(), "r");
    if (!fp) {
      return false;
    } else {
      fclose(fp);
      return 1;
    }
  }
  void Save(bool verbose=false) {
    if (FileExists(meta_filename)) {
      if (verbose)
        cout << "metadata file already exists." << endl;
      return;
    }
    // cout << "Saving..." << endl;

    FILE * fp = fopen(meta_filename.c_str(), "w");
    if (!fp) {
      cerr << "Error opening file '" << meta_filename << "' for writing." << endl;
      exit(EXIT_FAILURE);
    }
    if (1 != fwrite(&file_len, sizeof(file_len), 1, fp)) {
      cerr << "Error writing the variables" << endl;
      exit(1);
    }
    if (1 != fwrite(&line_len, sizeof(line_len), 1, fp)) {
      cerr << "Error writing the variables" << endl;
      exit(1);
    }
    if (1 != fwrite(&n_seqs, sizeof(n_seqs), 1, fp)) {
      cerr << "Error writing the variables" << endl;
      exit(1);
    }
    for (size_t i = 0; i < n_seqs; i++) {
      if (1 != fwrite(&(seq_lengths[i]), sizeof(seq_lengths[i]), 1, fp)) {
        cerr << "Error writing the variables" << endl;
        exit(1);
      }
    }
    for (size_t i = 0; i < n_seqs; i++) {
      if (1 != fwrite(&(headers_sp[i]), sizeof(headers_sp[i]), 1, fp)) {
        cerr << "Error writing the variables" << endl;
        exit(1);
      }
    }
    for (size_t i = 0; i < n_seqs; i++) {
      if (1 != fwrite(&(headers_ep[i]), sizeof(headers_ep[i]), 1, fp)) {
        cerr << "Error writing the variables" << endl;
        exit(1);
      }
    }

    for (size_t i = 0; i < n_seqs; i++) {
      size_t N = seq_names[i].length();
      if (1 != fwrite(&N, sizeof(N), 1 , fp)) {
        cerr << "Error writing string length" << endl;
        exit(1);
      }
      if (N != fwrite(seq_names[i].c_str(), 1, N , fp)) {
        cerr << "Error writing string content" << endl;
        exit(1);
      }
    }
    fclose(fp);
  }

  void Load(string _meta_filename) {
    // cout << "Loading..." << endl;
    cache_i = 0;
    meta_filename.assign(_meta_filename);
    n_seqs = 0;
    FILE * fp = fopen(meta_filename.c_str(), "r");
    if (!fp) {
      cerr << meta_filename << " cannot be open for loading, aborting." << endl;
      exit(EXIT_FAILURE);
    }
    if (1 != fread(&file_len, sizeof(file_len), 1, fp)) {
      cout << stderr << "Error reading file_len from file" << endl;
      exit(1);
    }
    if (1 != fread(&line_len, sizeof(line_len), 1, fp)) {
      cout << stderr << "Error reading line_len from file" << endl;
      exit(1);
    }
    if (1 != fread(&n_seqs, sizeof(n_seqs), 1, fp)) {
      cout << stderr << "Error reading n_seqs from file" << endl;
      exit(1);
    }
    seq_lengths.clear();
    seq_lengths.reserve(n_seqs);
    for (size_t i = 0; i < n_seqs; i++) {
      size_t tmp;
      if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
        cout << stderr << "Error reading  lengths from file" << endl;
        exit(1);
      }
      seq_lengths.push_back(tmp);
    }

    headers_sp.clear();
    headers_sp.reserve(n_seqs);
    for (size_t i = 0; i < n_seqs; i++) {
      size_t tmp;
      if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
        cout << stderr << "Error reading sps from file" << endl;
        exit(1);
      }
      headers_sp.push_back(tmp);
    }

    headers_ep.clear();
    headers_ep.reserve(n_seqs);
    for (size_t i = 0; i < n_seqs; i++) {
      size_t tmp;
      if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
        cout << stderr << "Error reading eps from file" << endl;
        exit(1);
      }
      headers_ep.push_back(tmp);
    }

    seq_names.clear();
    seq_names.reserve(n_seqs);
    for (size_t i = 0; i < n_seqs; i++) {
      size_t N;
      if (1 != fread(&N, sizeof(N), 1, fp)) {
        cout << stderr << "Error reading string length from file" << endl;
        exit(1);
      }
      string tmp_content(N, 0);
      if (N != fread(&tmp_content[0], 1, N, fp)) {
        cout << stderr << "Error reading string content from file" << endl;
        exit(1);
      }
      seq_names.push_back(tmp_content);
    }
    fclose(fp);
    BuildSeqLengthsSums();
  }

  // not thread-efficient. Each thread should have its own instance for the cache_i to make sense.
  bool is_within_header(size_t byte_pos) {
    // Beware: it assumes it is queried sequentially, otherwise it will fail.
    assert(byte_pos >= headers_sp[cache_i]);
    for (;
         cache_i < n_seqs-1 && 
         byte_pos >= headers_sp[cache_i + 1];
         cache_i++) {}
    assert(cache_i < n_seqs);
    assert(byte_pos >= headers_sp[cache_i]);
    assert(cache_i == n_seqs-1 || byte_pos < headers_sp[cache_i + 1] );
    return byte_pos <= headers_ep[cache_i];
  }
  
  size_t TotSeqLen() {
    return seq_lengths_sums[n_seqs];
  }

  size_t FindSeq(size_t abs_seq_i) {
    size_t i = 0;
    for (; i < n_seqs; i++) {
      assert(abs_seq_i >= seq_lengths_sums[i]);
      if (abs_seq_i < seq_lengths_sums[i+1])
        break;
    }
    return i;
  }
  
  size_t SeqposToFilepos(size_t abs_seq_i) {
    assert(abs_seq_i < TotSeqLen());
    size_t seq_id = FindSeq(abs_seq_i);
    size_t local_pos = abs_seq_i - seq_lengths_sums[seq_id];
    assert(local_pos < seq_lengths[seq_id]);
    
    if (line_len == LEN_UNLIMITED) {
      return headers_ep[seq_id] + 1 + local_pos;
    } else {
      size_t n_lines = local_pos / line_len;
      size_t n_chars = local_pos % line_len;
      size_t ans = headers_ep[seq_id] + 1 + n_chars + n_lines *(line_len + 1);
      return ans;
    }
  }

  void PrintInfo() {
    for (size_t i = 0; i < n_seqs; i++) {
      cout << "Seq " << i << endl;
      cout << "  name           : " << seq_names[i] << endl;
      cout << "  length         : " << seq_lengths[i] << endl;
      cout << "  header sp: " << headers_sp[i] << endl;
      cout << "  header ep: " << headers_ep[i] << endl;
    }
  }
  
  void Test() {
    PrintInfo();
    for (size_t i = 0; i < file_len; i++) {
      if  (this->is_within_header(i)){
        cout << endl << i << " shall be skipped" << endl;
      } else {
        cout << "*";
      }
    }
  }
  
  void BuildSeqLengthsSums() {
    seq_lengths_sums.clear();;
    seq_lengths_sums.reserve(n_seqs+1);
    size_t sum = 0;
    seq_lengths_sums.push_back(sum);
    for (size_t i = 0; i < n_seqs; i++) {
      sum+=seq_lengths[i];
      seq_lengths_sums.push_back(sum);
    }
  }

  virtual ~FastaMetaData() {
  }

 private:
  size_t file_len;
  size_t n_seqs;
  size_t line_len;
  vector<string> seq_names;
  vector<size_t> seq_lengths;
  vector<size_t> seq_lengths_sums;

  string fasta_filename;
  string meta_filename;

  size_t cache_i;
 public:
  vector<size_t> headers_sp;
  vector<size_t> headers_ep;
  size_t GetNSeqs() {
    return n_seqs;
  }
  size_t GetLineLength() {
    return line_len;
  }

  vector<string> * AccessSeqNames() {
    return &seq_names;
  }

  vector<size_t> * AccessSeqLengths() {
    return &seq_lengths;
  }
};
}
#endif /* METAFASTA_H */
