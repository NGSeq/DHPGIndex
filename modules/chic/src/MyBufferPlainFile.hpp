/*
 Daniel Valenzuela
 Simple buffer that offer an interface to go character by character from an array or from a file.
 */
#ifndef MYBUFFERPLAINFILE_H_ 
#define MYBUFFERPLAINFILE_H_ 

#include "MyBuffer.h"

class MyBufferPlainFile : public MyBuffer{
 public:
  MyBufferPlainFile(char * filename) {
    fs.open(filename);
    ASSERT(fs.good());
  }
  void SetPos(size_t j) {
    fs.seekg((int64_t)j);
  }
  inline uchar GetChar() {
    if(!fs.good()) {
      cout << "A PROBLEM OCCURRED WITH FS!! " << endl;
      exit(EXIT_FAILURE);
    }
    return  fs.get();
  }
  virtual ~MyBufferPlainFile() {
  }
 private:
  std::ifstream fs;
};
#endif /* MYBUFFERPLAINFILE_H_*/
