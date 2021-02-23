// Copyright Daniel Valenzuela
/**
 *This is chic alignment mapper for already kernel BWA aligned reads
 *The code is modified version of chic_align and discards BWA alignment phase
 * in KernelManagerBWA.cpp method KernelManagerBWA::LocateOccsFQ
 *
 * See also: HybridLZIndex void HybridLZIndex::FindPrimaryOccsFQ vs ..FQ2
 * Example pipeline:
 * 1. Align reads to kernel sequence with BWA MEM
 * 2. map SAM formatted alignments to original pan-genomic sequences with chic_map
 * 3. Use mapped SAMs for finding heaviest path for adhoc genome.
 * Usage: chic_map <lzindex> <alignmentfile>
 **/
#include <getopt.h>
#include <sdsl/util.hpp>
#include <sdsl/vectors.hpp>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include "./HybridLZIndex.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

using std::ifstream;

void suggest_help();
void suggest_help(char ** argv) {
  cout << "For help, type " << argv[0] << " --help" << endl;
}

void print_help();
void print_help() {
  cout << "Compressed Hybrid Index v0.1 beta" << endl;
  cout << "chic_align aligns reads in fq format" << endl;
  cout << endl;
  cout << "Ussage: chic_align [OPTIONS] <index_basename> <reads1.fq> (reads2.fq)" << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "-o --output=OUTPUT_FILENAME Default: Standard output" << endl;
  cout << "-s --secondary_report=[ALL|LZ|NONE] Default=NONE" << endl;
  cout << "-t --threads=(number of threads)" << endl;
  cout << "-p --interleaved-reads " << endl;
  cout << "-K --kernel-options " << endl;
  cout << "-v --verbose=LEVEL " << endl;
  cout << "--help " << endl;
}

typedef struct {
  // Required:
  char * index_basename;
  char * alignment_filename;
  // Options:
  bool interleaved_mates;
  InputType input_type;
  char * output_filename;
  SecondaryReportType secondary_report;
  int n_threads;
  int verbose;
  vector<string> kernel_options;
} Parameters;


int global_correct = 0;
void FailExit();
void Success();

void FailExit() {
  printf(ANSI_COLOR_RED "\tTest Failed\n\n" ANSI_COLOR_RESET);
  exit(1);
}
void Success() {
  printf(ANSI_COLOR_BLUE "\t\tSuccess\n" ANSI_COLOR_RESET);
}

vector<string> LoadPatterns(char * filename, uint max_query_len);

int main(int argc, char **argv) {


    if (argc < 3) {
        fprintf(stderr, "%s <infile1> <infile2> ... <infileN> \n\n"
                        "It merges the metadata parsings of infile1 to infileN, correcting by adding the cumulated offset. Output is written to merged.meta\n", argv[0]);
        std::exit(EXIT_FAILURE);
    }

    int n_files = argc - 1;

    int opt = atoi(argv[1]);

    if(opt==0) {

        size_t lsize = 0;
        for (int file_id = 2; file_id <= n_files; file_id++) {
            enc_vector<elias_delta, REGULAR_DENS> lk;
            load_from_file(lk, argv[file_id]);
            lsize += lk.size();
        }
        cout << "totlsize " << lsize << endl;
        int_vector<> tmp_limits_int_vector(lsize);

        uint64_t offset = 0;
        for (int file_id = 2; file_id <= n_files; file_id++) {
            //uint64_t byte_len = utils::file_size(argv[file_id]);

            enc_vector<elias_delta, REGULAR_DENS> limits_kernel;

            load_from_file(limits_kernel, argv[file_id]);

            //size_t seq_len = byte_len;
            //FILE * fp = std::fopen(argv[file_id], "r");

            uint64_t last = 0;
            size_t s = limits_kernel.size();
            cout << "lsize " << s << endl;

            size_t i = 0;
            for (i = 0; i < s; i++) {
                //cout << i << "=i " <<endl;
                //cout << limits_kernel[i] << endl;
                if (file_id == 3 && last == 0)
                    cout << "skip first " << endl;
                else tmp_limits_int_vector[i] = limits_kernel[i] + offset;
                last = limits_kernel[i];
            }
            offset += last;

            size_t ofs = offset;
            cout << "offs " << ofs << endl;

        }

        store_to_file(enc_vector<elias_delta, REGULAR_DENS>(tmp_limits_int_vector), "merged.limits_kernel");
    }else{
        size_t lsize = 0;
        size_t sparse_sample_ratio = 512;

        enc_vector<elias_delta, REGULAR_DENS> lk;
            load_from_file(lk, argv[2]);
            size_t nphrases = lk.size();

        cout << "nPhrases " << nphrases << endl;

        size_t sparse_sample_limits_kernel_len = (nphrases)/sparse_sample_ratio;
        if ((nphrases)%sparse_sample_ratio)
            sparse_sample_limits_kernel_len++;
        vector<uint64_t> tmp_limits_int_vector = vector<uint64_t>(sparse_sample_limits_kernel_len);

            //uint64_t byte_len = utils::file_size(argv[file_id]);

            //size_t seq_len = byte_len;
            //FILE * fp = std::fopen(argv[file_id], "r");

            uint64_t posFil = 0;
            uint64_t countSMSucc = 0;

            for (size_t i = 0; i < nphrases; i++) {
                posFil = lk[i];
                if (i%sparse_sample_ratio == 0) {
                    //cout << "nf " << posFil << endl;
                    tmp_limits_int_vector[countSMSucc] = posFil;
                    countSMSucc++;
                }
                //tmp_limits_int_vector[i] = tmp_limits_kernel[i];
            }


        store_to_file(tmp_limits_int_vector, "merged.sparse_limits_kernel");
    }

}


