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

#ifndef BOOK_KEEPER_H_
#define BOOK_KEEPER_H_

#include <vector>
#include <string>
#include <zlib.h>
#include "./utils.h"
#include "./occurrence.h"

class BookKeeper {
  public:
    BookKeeper();
    BookKeeper(char * filename, KernelType kernel_type, int _verbose);
    BookKeeper(char * filename, KernelType kernel_type, uchar ** seq, size_t * seq_len, int _verbose);
    void ReadPlain(char * filename, uchar ** seq_ans, size_t * seq_len_ans);
    void ReadFasta(char * filename, uchar ** seq_ans, size_t * seq_len_ans);
    void FilterFasta(char * input_filename);
    void CreateMetaData(char * filename);
    
    virtual ~BookKeeper();

    void SetFileNames(char * index_prefix);
    vector<string> SamHeader();
    void Load(char * index_prefix, int _verbose);
    void Save() const;
    char * GetNewFileName();
    void NormalizeOutput(Occurrence& occ);
    
    char bk_filename[200];


    size_t GetTotalLength() {
      return total_length;
    }

  private:
    int verbose;
    //string new_input_name;
    size_t n_seqs;
    size_t total_length;
    vector<string> seq_names;
    vector<size_t> seq_lengths;
};

#endif /* BOOK_KEEPER_H_ */
