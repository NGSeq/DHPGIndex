#ifndef HYBRID_LZINDEX_H_
#define HYBRID_LZINDEX_H_

#include "RangeReporting.h"
#include "KernelManagerBWA.h"
#include "KernelManagerBowTie2.h"
#include "KernelManagerBLAST.h"
#include "KernelManagerFMI.h"
#include "BookKeeper.h"
#include "MyBuffer.h"
#include "MyBufferPlainFile.hpp"
#include "MyBufferMemSeq.hpp"
#include "MyBufferFastaFile.hpp"
#include "MyBufferHDFS.hpp"
#include <streambuf>
#include <ostream>
#include <sdsl/util.hpp>
#include <sdsl/io.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include "./jni/hdfs.h"


using std::ostream;

using sdsl::int_vector;
using sdsl::enc_vector;
using sdsl::coder::elias_delta;
using sdsl::store_to_file;
using sdsl::load_from_file;
using sdsl::csa_wt;
using sdsl::wt_huff;
using sdsl::rrr_vector;
using sdsl::construct;

class HybridLZIndex {
 public:
  // INDEX CONSTRUCTION
  // Three ways to build the index::
  HybridLZIndex();
  void Load(char * _prefix,
            int n_threads,
            int _verbose);

  HybridLZIndex(BuildParameters * parameters);
  
  virtual ~HybridLZIndex();
  // Used accessors:
  uchar * GetTmpSeq() {
    return tmp_seq;
  }
  size_t GetTextLength() {
    return text_len;
  }

  char * GetTextFileName() {
    return text_filename;
  }

  char * GetInputLZFilename() {
    return input_lz_filename;
  }

  LZMethod GetLZMethod() {
    return lz_method;
  }

  int GetMaxMemoryMB() {
    return max_memory_MB;
  }

  uint GetMaxQueryLen() {
    return max_query_len;
  }
  
  int GetNThreads() {
    return n_threads;
  }

  int GetRLZRefLength() {
    return rlz_ref_len_MB; 
  }
  void ValidateParams(BuildParameters * params);
  void WriteKernelTextFile(uchar * _kernel_text, size_t _kernel_text_len);
  void WriteToHDFS(uchar * text);
  /////////////////////
  // INDEX QUERIES
/////////////////////
  void FindFQ(char * query_filename,
              char * mates_filename,
              bool single_file_paired,
              SecondaryReportType secondary_report,
              vector<string> kernel_options,
              ostream& out_stream) const;

  void FindFQ2(char * alignment_filename,
                bool single_file_paired,
                SecondaryReportType secondary_report,
                vector<string> kernel_options,
                ostream& out_stream) const;

  void FindALL(vector<Occurrence> my_occs,
                char * query_filename,
                char * mates_filename,
                 bool single_file_paired,
                 SecondaryReportType secondary_report,
                 vector<string> kernel_options,
                 ostream& out_stream) const;

  void Find(vector<uint64_t> * ans, string query) const;
  void Find(vector<string> *ans, vector<uint64_t> position, uint64_t range) const;
  void DetailedSpaceUssage() const;
  uint GetSizeBytes() const;
  void Save() const;

    void FindPatterns(vector<string> *ans, string query) const;

    void FindPatterns(vector<Occurrence> *ans, string query) const;

private:

  void ComputeSize();
  void Build();
  void SetFileNames();
  void GetLZPhrases(vector<pair<uint64_t, uint>> * lz_phrases_ptr);
  void Kernelize();
  void InitKernelizeonly();
  void Kernelizeonly();
  void IndexingOnly();
  void IndexKernel();
  void Indexing();
  void MakeKernelString(MyBuffer *is, uchar ** kernel_ans, uint64_t ** tmp_limits_kernel_ans);
  void EncodeKernelLimitsAndSuccessor(uint64_t * tmp_limits_kernel);

  // others..
  void ChooseSpecialSeparator(uchar *seq);
  void ChooseSpecialSeparator(char * filename);
  void SetSpecialSeparator(uint64_t * alpha_test_tmp);
  void ComputeKernelTextLen();

  // Find:
  void FindPrimaryOccs(vector<Occurrence> * ans, string query) const;
  void searchSecondaryOcc(vector<Occurrence> * ans, uint *nSec = NULL) const;

  void FindPrimaryOccsFQ(vector<Occurrence> * ans,
                         vector<Occurrence> * unmapped,
                         SecondaryReportType secondary_report,
                         char * query_filename,
                         char * mates_filename,
                         bool single_file_paired,
                         vector<string> kernel_options) const;
  void FindPrimaryOccsFQ2(vector<Occurrence> * ans,
                           vector<Occurrence> * unmapped,
                           SecondaryReportType secondary_report,
                           char * alignment_filename,
                           bool single_file_paired,
                           vector<string> kernel_options) const;

  void CreateSamRecordsForTrulyLostAlignments(vector<Occurrence> * lost_occs,
                                              vector<Occurrence> * unmapped,
                                              vector<Occurrence> * ans,
                                              bool retrieve_all) const;

  uint64_t MapKernelPosToTextPos(uint64_t pos, uint64_t * next_limit_pos, uint * predecessor_i) const;
  uint SuccessorInKernelLimits(uint64_t x, uint64_t * val) const;
  uint SampleBinarySearch(uint64_t x) const;
  uint SuccessorBinarySearch(uint64_t x, uint l, uint r, uint64_t *val) const;

  // ACCESSORS:
  uint64_t GetLimitKernel(uint pos) const;
  uint64_t GetLimit(uint pos) const;


  ///////////////////////////////////////////////////////////////////
  /////////// Member Variables
  ///////////////////////////////////////////////////////////////////
 private:
  BookKeeper * book_keeper;
  KernelType kernel_type;
  LZMethod lz_method;
  int max_memory_MB;
  int rlz_ref_len_MB;
  int n_threads;

  uint64_t text_len;
  uint64_t kernel_text_len;
  uchar special_separator;

  uint max_query_len;
  uint max_insertions;
  uint context_len;
  uint sparse_sample_ratio;

  RangeReporting *tsrr;

  vector<uint64_t> sparse_sample_limits_kernel;  // for LimitsKernel


  uint n_phrases;
  enc_vector<elias_delta,REGULAR_DENS> limits_kernel;

  //KernelManagerBWA * kernel_manager;
  KernelManager * kernel_manager;
  int verbose;
  uint index_size_in_bytes;
  uint sigma;
  uchar * tmp_seq;  // ptr to seq, in case it was provided.

  char * text_filename;
  char * hdfs_path;
  char * index_prefix;
  char * input_lz_filename;
  // intermediate files for index construction
  char kernel_manager_prefix[200];
  // prefix for index file names:
  // index file names:
  char sparse_sample_limits_kernel_filename[200];
  char limits_kernel_filename[200];

  char variables_filename[200];

  bool InspectIndex();
hdfsFS fs;

};

#endif /* LZ77_MINDEX_H_ */
