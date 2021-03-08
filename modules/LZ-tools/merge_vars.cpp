#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>
#include "utils.h"
#define uchar unsigned char

using std::pair;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::string;

enum class KernelType { BWA, BOWTIE2, FMI };
typedef struct {
    KernelType kernel_type;
} Ktype;
// We assume input is LZ files using uint64_t for each number.
int main(int argc, char **argv) {


    size_t n_seqst = 0;
    vector<size_t>  seq_lens_bff;
    vector<size_t>  headers_sp_bff;
    vector<size_t>  headers_ep_bff;
    //vector<size_t> seq_names_len_bff = new vector<size_t>[n_seqst];
    vector<string>  seq_names_bff;
    vector<size_t>  seq_lengths_sums_bff;

    if (argc < 3) {
        fprintf(stderr, "%s <infile1> <infile2> ... <infileN> \n\n"
                        "It merges the metadata parsings of infile1 to infileN, correcting by adding the cumulated offset. Output is written to merged.meta\n", argv[0]);
        std::exit(EXIT_FAILURE);
    }

    int n_files = argc - 1;
    uint64_t cumulated_offset = 0;


    KernelType kernel_type;

    uint64_t text_len;
    uint64_t kernel_text_len;
    uchar special_separator;

    uint max_query_len;
    uint max_insertions;
    uint context_len;
    uint sparse_sample_ratio;


    for (int file_id = 1; file_id <= n_files; file_id++) {
        FILE * fp = std::fopen(argv[file_id], "r");
        if (1 != fread(&text_len, sizeof(text_len), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        if (1 != fread(&special_separator, sizeof(special_separator), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        if (1 != fread(&max_query_len, sizeof(max_query_len), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        if (1 != fread(&max_insertions, sizeof(max_insertions), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        context_len = max_query_len + max_insertions;

        if (1 != fread(&sparse_sample_ratio, sizeof(sparse_sample_ratio), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        if (1 != fread(&kernel_type, sizeof(kernel_type), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        if (1 != fread(&kernel_text_len, sizeof(kernel_text_len), 1, fp)) {
            cerr << stderr << "Error reading var from file" << endl;
            exit(1);
        }
        cout << "text_len " << text_len << endl;
        cout << special_separator << endl;
        cout << "max_query_len " << max_query_len << endl;
        cout << "max_insertions " << max_insertions << endl;
        cout << "sparse_sample_ratio " << sparse_sample_ratio << endl;
        cout << "kernel_text_len " << kernel_text_len << endl;

        fclose(fp);
    }


}


//file_len,line_len,n_seqs,seq_lengths[n_seqs],headers_sp[n_seqs],headers_ep[n_seqs],seq_names[n_seqs].length(),seq_names[n_seqs].c_str()

