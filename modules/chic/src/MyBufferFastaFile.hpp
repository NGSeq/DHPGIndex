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

#ifndef MYBUFFERFASTAFILE_H_ 
#define MYBUFFERFASTAFILE_H_ 

#include "MyBuffer.h"
#include "../ext/LZ/RLZ_parallel/src/fastametadata.hpp"

using FMD::FastaMetaData;

class MyBufferFastaFile : public MyBuffer{
 public:
  MyBufferFastaFile(char * filename) {
    metadata = new FastaMetaData();
    metadata->Load(string(filename)+".metadata");
    line_len = metadata->GetLineLength();
    fs.open(filename);
    ASSERT(fs.good());
    seq_i = 0;
    file_i = metadata->SeqposToFilepos(0);
    fs.seekg((int64_t)file_i);
  }

  bool AmGood() {
    //ASSERT(!metadata->is_within_header(file_i));
    ASSERT(file_i >= seq_i);
    ASSERT(metadata->SeqposToFilepos(seq_i) == file_i);
    return true;
  }

  void SetPos(size_t seq_j) {
    ASSERT(seq_j >= seq_i);
    //ASSERT(AmGood());
    
    seq_i = seq_j;
    file_i = metadata->SeqposToFilepos(seq_i);
    fs.seekg((int64_t)file_i);
    
    ASSERT(AmGood());
  }

  inline uchar GetChar() {
    //ASSERT(AmGood());
    if(!fs.good()) {
      cerr << "A PROBLEM OCCURRED WITH FS!! " << endl;
      exit(EXIT_FAILURE);
    }
    uchar curr_char = fs.get();
    seq_i++;
    file_i++;
    while (metadata->is_within_header(file_i) || Utils::IsNewLine(curr_char)) {
      curr_char = fs.get();
      file_i++;
    }
    return  curr_char;
  }
  ~MyBufferFastaFile() {
    delete(metadata);
  }
 private:
  FastaMetaData *metadata;
  std::ifstream fs;
  size_t file_i;
  size_t seq_i;
  size_t line_len;
};
#endif /* MYBUFFERFASTAFILE_H_*/
