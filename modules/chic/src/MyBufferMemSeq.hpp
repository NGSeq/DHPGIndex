/*
 Daniel Valenzuela
 Simple buffer that offer an interface to go character by character from an array or from a file.
 */
#ifndef MYBUFFERMEMSEQ_H_ 
#define MYBUFFERMEMSEQ_H_

#include "MyBuffer.h"

class MyBufferMemSeq : public MyBuffer{
  public:
    MyBufferMemSeq(uchar * _seq, size_t _len) {
        fprintf(stdout, " MEMSEQ");
      seq = _seq;
      len = _len;
      i = 0;
    }
    void SetPos(size_t j) {
      ASSERT(seq != NULL);
      ASSERT(j >= i);
      i = j;
    }
    inline uchar GetChar() {
      ASSERT(seq != NULL);
        ASSERT(i < len);
        uchar ans = seq[i];
        i++;
        //fprintf(stdout, " %c ", ans);
        return ans;
    }
    virtual ~MyBufferMemSeq() {
    }
  private:
    //std::ifstream fs;
    size_t i;
    uchar * seq;
    size_t len;
};
#endif /* MYBUFFERMEMSEQ_H_ */
