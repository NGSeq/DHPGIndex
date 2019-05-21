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

#include "./BookKeeper.h"
#include <string>
#include <cstring>
#include <vector>
#include "./utils.h"
#include "./fasta.h"
#include "../ext/LZ/RLZ_parallel/src/fastametadata.hpp"

using FMD::FastaMetaData;

BookKeeper::BookKeeper() {
  n_seqs = 0;
}

// EM, RLZ, or actually anything that is not IM is calling this one:
// We work, for now, undet the assumtion that each chromosome fits in main
// memory, even though is possible that the entire file does not.
BookKeeper::BookKeeper(char * input_filename,
                       KernelType kernel_type,
                       int _verbose) {
  this->verbose = _verbose;
  if (kernel_type == KernelType::FMI) {
		// Currently, when using FMI for the kernel sequence we handle only a single input:
    n_seqs = 0;  
    total_length = Utils::GetLength(input_filename);
  } else if (Utils::IsBioKernel(kernel_type)) {
    CreateMetaData(input_filename);
  } else {
    cerr << "Don't know how to manage kernel type" << endl;
    exit(EXIT_FAILURE);
  }
}


// In Memory construction of Book Keeper
BookKeeper::BookKeeper(char * input_filename,
                       KernelType kernel_type,
                       uchar ** seq_ans,
                       size_t * seq_len_ans,
                       int _verbose) {
  this->verbose = _verbose;
  if (kernel_type == KernelType::FMI) {
    ReadPlain(input_filename, seq_ans, seq_len_ans);
  } else if (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2) {
    ReadFasta(input_filename, seq_ans, seq_len_ans);
  } else {
    cerr << "Unknown kernel type" << endl;
    exit(EXIT_FAILURE);
  }
}

void BookKeeper::CreateMetaData(char * filename) {
  FastaMetaData *metadata;
  metadata = new FastaMetaData(string(filename));
  metadata->Save();

  n_seqs = metadata->GetNSeqs();

  seq_lengths.clear();
  seq_lengths.reserve(n_seqs);

  seq_names.clear();
  seq_names.reserve(n_seqs);

  vector<size_t> * ptr_lengths = metadata->AccessSeqLengths();
  vector<string> * ptr_names = metadata->AccessSeqNames();
  total_length = 0;
  for (size_t i = 0; i < n_seqs; i++) {
    seq_names.push_back(ptr_names->at(i));
    seq_lengths.push_back(ptr_lengths->at(i));
    total_length += seq_lengths[i];
  }
  delete(metadata);
}

void BookKeeper::ReadPlain(char * filename, uchar ** seq_ans, size_t * seq_len_ans) {
  FILE *infile = Utils::OpenReadOrDie(filename);
  fseek(infile, 0, SEEK_END);
  size_t len  = (size_t)ftell(infile);
  fseek(infile, 0, SEEK_SET);
  uchar * seq = new uchar[len + 1];
  if (len != fread(seq, sizeof(uchar), len, infile)) {
    cerr << "Error reading string from file" << endl;
    exit(1);
  }
  seq[len] = 0;
  fclose(infile);
  cerr << " [0].- Text [1.." << len << "] Read !! " << endl;
  *seq_ans =  seq;
  *seq_len_ans = len;

  n_seqs = 1;
  seq_lengths.push_back(len);
  seq_names.push_back(string(filename));
}

void BookKeeper::ReadFasta(char * filename, uchar ** seq_ans, size_t * seq_len_ans) {
  FASTAFILE *ffp;
  char *seq;
  char *name;
  size_t   L;
  size_t byte_length = Utils::GetLength(filename);
  uchar * ans = new uchar[byte_length];
  ffp = OpenFASTA(filename);
  total_length = 0;
  n_seqs = 0;
  seq_names.clear();
  seq_lengths.clear();
  while (ReadFASTA(ffp, &seq, &name, &L)) {
    for (size_t i = 0; i < L; i++) {
      ans[total_length+i] = (uchar)seq[i];
    }
    total_length += L;
    n_seqs++;
    seq_lengths.push_back(L);
    seq_names.push_back(string(name));

    free(seq);
    free(name);
  }
  CloseFASTA(ffp);
  *seq_ans = ans;
  *seq_len_ans = total_length;
}

/*
void BookKeeper::FilterFasta(char * filename) {
  char * output_name = this->GetNewFileName();
  FILE * fp = Utils::OpenWriteOrDie(output_name);

  FASTAFILE *ffp;
  char *seq;
  char *name;
  size_t   L;
  ffp = OpenFASTA(filename);
  size_t tot_length = 0;
  n_seqs = 0;
  seq_names.clear();
  seq_lengths.clear();
  while (ReadFASTA(ffp, &seq, &name, &L)) {
    if (L != fwrite(seq, 1, L, fp)) {
      cerr << "Error writing to the raw sequence file" << endl;
      exit(1);
    }
    tot_length += L;
    n_seqs++;
    seq_lengths.push_back(L);
    seq_names.push_back(string(name));

    free(seq);
    free(name);
  }
  CloseFASTA(ffp);

  fclose(fp);
  delete[] output_name;
}
*/
/*
char * BookKeeper::GetNewFileName() {
  size_t len = new_input_name.length();
  char * ans = new char[len + 1];
  strcpy(ans, new_input_name.c_str());
  return ans;
}
*/

void BookKeeper::NormalizeOutput(Occurrence& occ) {
  if (occ.IsUnmapped())
    return;
  size_t kernel_pos = occ.GetPos();
  size_t next_limit = 0;
  size_t doc_id;
  ASSERT(n_seqs == seq_lengths.size());
  size_t offset = 0;
  for (doc_id = 0; doc_id < seq_lengths.size(); doc_id++) {
    offset = next_limit;
    next_limit += seq_lengths[doc_id];
    if (kernel_pos < next_limit) {
      break;
    }
  }
  ASSERT(doc_id < n_seqs);
  ASSERT(offset <= kernel_pos);
  ASSERT(kernel_pos < next_limit);
  size_t new_pos = kernel_pos - offset;
  if (new_pos + occ.GetLength() <= seq_lengths[doc_id]) {
    occ.UpdatePos(new_pos, seq_names[doc_id]);
  } else {
    if (verbose >= 2)
      cerr << "Warning: Read alignment that overlaps two chromomses will be discarded." << endl;
    // This read got aligned overlaping two chromosomes. Current solution is to mark it unmapped.
    occ = Occurrence(Utils::SamRecordForUnmapped(occ.GetReadName()));
  }
}

void BookKeeper::SetFileNames(char * _prefix) {
  sprintf(bk_filename, "%s.book_keeping", _prefix);
}

void BookKeeper::Save() const {
  FILE * fp = Utils::OpenWriteOrDie(bk_filename);
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

void BookKeeper::Load(char * index_prefix, int _verbose) {
  this->verbose = _verbose;
  SetFileNames(index_prefix);
  FILE * fp = Utils::OpenReadOrDie(bk_filename);
  if (1 != fread(&n_seqs, sizeof(n_seqs), 1, fp)) {
    cerr << stderr << "Error reading var from file" << endl;
    exit(1);
  }
  seq_lengths.clear();
  seq_lengths.reserve(n_seqs);
  for (size_t i = 0; i < n_seqs; i++) {
    size_t tmp;
    if (1 != fread(&tmp, sizeof(tmp), 1, fp)) {
      cerr << stderr << "Error reading var from file" << endl;
      exit(1);
    }
    seq_lengths.push_back(tmp);
  }

  seq_names.clear();
  seq_names.reserve(n_seqs);
  for (size_t i = 0; i < n_seqs; i++) {
    size_t N;
    if (1 != fread(&N, sizeof(N), 1, fp)) {
      cerr << stderr << "Error reading string length from file" << endl;
      exit(1);
    }
    string tmp_content(N, 0);
    if (N != fread(&tmp_content[0], 1, N, fp)) {
      cerr << stderr << "Error reading string content from file" << endl;
      exit(1);
    }
    seq_names.push_back(tmp_content);
  }
  fclose(fp);
}

vector<string> BookKeeper::SamHeader() {
  vector<string> ans;
  for (size_t i = 0; i < n_seqs; i++) {
    string curr = "@SQ\tSN:" + seq_names[i] + "\tLN:" + std::to_string(seq_lengths[i]);
    ans.push_back(curr);
  }
  return ans;
}

BookKeeper::~BookKeeper() {
}
