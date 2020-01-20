/*
 Daniel Valenzuela
 Simple buffer that offer an interface to go character by character from an array or from a file.
 */
#ifndef MYBUFFERMIX_H_ 
#define MYBUFFERMIX_H_ 

#include "MyBuffer.h"

class MyBufferMix : public MyBuffer{
  public:
    MyBufferMix(char * filename) {
        fprintf(stdout, " MIX");

        fs.open(filename);
      ASSERT(fs.good());
      seq = NULL;
    }
    MyBufferMix(uchar * _seq, size_t _len) {
        fprintf(stdout, " MIX");

        seq = _seq;
      len = _len;
      i = 0;
    }
    void SetPos(size_t j) {
      if (seq != NULL) {
        ASSERT(j >= i);
        i = j;
      } else {
        fs.seekg((int64_t)j);
      }
    }
    inline uchar GetChar() {
      if (seq != NULL) {
        ASSERT(i < len);
        uchar ans = seq[i];
        i++;
        return ans;
      } else {
        if(!fs.good()) {
          cout << "A PROBLEM OCCURRED WITH FS!! " << endl;
          exit(EXIT_FAILURE);
        }
        return  fs.get();
      }
    }
    virtual ~MyBufferMix() {
    }
  private:
    std::ifstream fs;
    size_t i;
    uchar * seq;
    size_t len;
    // stream.
};
#endif /* MYBUFFERMIX_H_*/
