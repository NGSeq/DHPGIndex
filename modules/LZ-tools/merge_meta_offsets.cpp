#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>
#include "utils.h"

using std::pair;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::string;

// We assume input is LZ files using uint64_t for each number.
int main(int argc, char **argv) {


    size_t n_seqst = 0;
    size_t file_len_bff;
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


    size_t file_len;
    size_t n_seqs;
    size_t line_len;
    vector<string> seq_names;

    string fasta_filename;
    string meta_filename;

    size_t cache_i;

    vector<size_t> headers_sp;
    vector<size_t> headers_ep;

    for (int file_id = 1; file_id <= n_files; file_id++) {
        FILE * fp = utils::open_file(argv[file_id], "r");
        if (1 != fread(&file_len, sizeof(file_len), 1, fp)) {
            cout << stderr << "Error reading file_len from file" << endl;
            exit(1);
        }
        cout << "file_len " << file_len << endl;

        if (1 != fread(&line_len, sizeof(line_len), 1, fp)) {
            cout << stderr << "Error reading line_len from file" << endl;
            exit(1);
        }
        cout << "line_len " << line_len << endl;

        if (1 != fread(&n_seqs, sizeof(n_seqs), 1, fp)) {
            cout << stderr << "Error reading n_seqs from file" << endl;
            exit(1);
        }
        cout << "n_seqs " << n_seqs << endl;

        n_seqst += n_seqs;
        cout << "n_seqst " << n_seqst << endl;
    }

//file_len,line_len,n_seqs,seq_lengths[n_seqs],headers_sp[n_seqs],headers_ep[n_seqs],seq_names[n_seqs].length(),seq_names[n_seqs].c_str()


    for (int file_id = 1; file_id <= n_files; file_id++) {

        FILE * fp = utils::open_file(argv[file_id], "r");

        if (1 != fread(&file_len, sizeof(file_len), 1, fp)) {
            cout << stderr << "Error reading file_len from file" << endl;
            exit(1);
        }
        file_len_bff += file_len;

        if (1 != fread(&line_len, sizeof(line_len), 1, fp)) {
            cout << stderr << "Error reading line_len from file" << endl;
            exit(1);
        }
        if (1 != fread(&n_seqs, sizeof(n_seqs), 1, fp)) {
            cout << stderr << "Error reading n_seqs from file" << endl;
            exit(1);
        }

        for (size_t i = 0; i < n_seqs; i++) {
            size_t tmp;
            if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
                cout << stderr << "Error reading  lengths from file" << endl;
                exit(1);
            }
            seq_lens_bff.push_back(tmp);
        }

        headers_sp.clear();
        headers_sp.reserve(n_seqs);
        for (size_t i = 0; i < n_seqs; i++) {
            size_t tmp;
            if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
                cout << stderr << "Error reading sps from file" << endl;
                exit(1);
            }
            //cout << "headers_sp " << tmp << endl;
            size_t hsp = tmp;
            if(file_id>1)
                hsp = tmp+seq_lengths_sums_bff[seq_lengths_sums_bff.size()-1];
            //cout << "headers_sp-buff " << hsp << endl;

            headers_sp_bff.push_back(hsp);
        }

        headers_ep.clear();
        headers_ep.reserve(n_seqs);
        for (size_t i = 0; i < n_seqs; i++) {
            size_t tmp;
            if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
                cout << stderr << "Error reading eps from file" << endl;
                exit(1);
            }
            //cout << "headers_ep " << tmp << endl;
            size_t hep = tmp;
            if(file_id>1)
                hep = tmp+seq_lengths_sums_bff[seq_lengths_sums_bff.size()-1];
            //cout << "headers_ep-buff " << hep << endl;

            headers_ep_bff.push_back(hep);
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
            seq_names_bff.push_back(tmp_content);
        }

        fclose(fp);

        //BuildSeqLengthsSums
        size_t sum = 0;
        if (file_id>1)
            sum=seq_lengths_sums_bff[seq_lengths_sums_bff.size()-1];
        seq_lengths_sums_bff.push_back(sum);
        for (size_t i = 0; i < n_seqs; i++) {
            sum+=seq_lens_bff[i];
            seq_lengths_sums_bff.push_back(sum);
            //cout <<  "seq_lengths_sums_bff" << sum << endl;
        }
    }
    cout <<  "seq_lengths_sums_bff size" << seq_lengths_sums_bff.size() << endl;
    cout <<  "seq_lengths_sums_bff" << seq_lengths_sums_bff[seq_lengths_sums_bff.size()-1] << endl;



    FILE * fpw = utils::open_file("merged.meta", "w");
    if (!fpw) {
        cerr << "Error opening file '" << meta_filename << "' for writing." << endl;
        exit(EXIT_FAILURE);
    }
    if (1 != fwrite(&file_len_bff, sizeof(file_len_bff), 1, fpw)) {
        cerr << "Error writing the variables" << endl;
        exit(1);
    }
    if (1 != fwrite(&line_len, sizeof(line_len), 1, fpw)) {
        cerr << "Error writing the variables" << endl;
        exit(1);
    }
    if (1 != fwrite(&n_seqst, sizeof(n_seqst), 1, fpw)) {
        cerr << "Error writing the variables" << endl;
        exit(1);
    }
    for (size_t i = 0; i < n_seqst; i++) {
        if (1 != fwrite(&(seq_lens_bff[i]), sizeof(seq_lens_bff[i]), 1, fpw)) {
            cerr << "Error writing the variables" << endl;
            exit(1);
        }
    }
    for (size_t i = 0; i < n_seqst; i++) {
        if (1 != fwrite(&(headers_sp_bff[i]), sizeof(headers_sp_bff[i]), 1, fpw)) {
            cerr << "Error writing header sp variables" << endl;
            exit(1);
        }
    }
    for (size_t i = 0; i < n_seqst; i++) {
        if (1 != fwrite(&(headers_ep_bff[i]), sizeof(headers_ep_bff[i]), 1, fpw)) {
            cerr << "Error writing header ep variables" << endl;
            exit(1);
        }
    }

    for (size_t i = 0; i < n_seqst; i++) {
        size_t N = seq_names_bff[i].length();
        if (1 != fwrite(&N, sizeof(N), 1 , fpw)) {
            cerr << "Error writing string length" << endl;
            exit(1);
        }
        if (N != fwrite(seq_names_bff[i].c_str(), 1, N , fpw)) {
            cerr << "Error writing string content" << endl;
            exit(1);
        }
    }
    fclose(fpw);


}

//file_len,line_len,n_seqs,seq_lengths[n_seqs],headers_sp[n_seqs],headers_ep[n_seqs],seq_names[n_seqs].length(),seq_names[n_seqs].c_str()

