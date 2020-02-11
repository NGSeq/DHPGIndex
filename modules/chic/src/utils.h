// Copyright Daniel Valenzuela
#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdlib.h>
#include <sys/resource.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

#ifdef NDEBUG
#define ASSERT(x) do { (void)sizeof(x);} while (0)
#else
#include <assert.h>
#define ASSERT(x) assert(x)
#endif

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

enum class KernelType { BWA, BOWTIE2, BLAST, FMI };
enum class InputType { PLAIN, FQ };
enum class SecondaryReportType { ALL, LZ, NONE };
enum class LZMethod {IN_MEMORY, EXTERNAL_MEMORY, RLZ, INPUT};

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::string;
using std::tuple;

typedef struct {
  char * input_filename;
  char * output_filename;
  char * input_lz_filename;
  LZMethod lz_method;
  int n_threads;
  int mem_limit_MB;
  int rlz_ref_len_MB;
  uint max_query_len;
  uint max_edit_distance;
  KernelType kernel_type;
  int verbose;
  int kernelizeonly;
  int indexingonly;
  char * hdfs_path;
} BuildParameters;

namespace Utils {
  long double wclock();
  FILE * OpenWriteOrDie(const char * filename);
  FILE * OpenWriteOrDie(char * filename);
  FILE * OpenReadOrDie(const char * filename);
  FILE * OpenReadOrDie(char * filename);
  size_t GetLength(const char * filename);
  size_t GetLength(char * filename);
  string RandomString();
  void DeleteTmpFile(string filename);
  void DeleteFile(string filename);
  size_t CigarToLen(string cigar);
  size_t CigarSoftClipped(string cigar);
  string SamRecordForUnmapped(string read_name);
  bool IsBioKernel(KernelType kernel_type);
  bool IsNewLine(uchar c);
}  // namespace Utils

#endif  // SRC_UTILS_H_

