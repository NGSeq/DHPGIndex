/*
   Daniel Valenzuela
   Simple buffer that offer an interface to go character by character from an array or from a file.
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
      cout << "A PROBLEM OCCURRED WITH FS!! " << endl;
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
