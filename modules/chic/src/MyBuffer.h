/*
 Daniel Valenzuela
 Simple buffer that offer an interface to go character by character from an array or from a file.
 */

#ifndef MYBUFFER_H_ 
#define MYBUFFER_H_ 

class MyBuffer {
  public:
    MyBuffer() {
    };
    virtual void SetPos(size_t j) = 0;
    virtual uchar GetChar() = 0;
    virtual ~MyBuffer() {};
};
#endif /* MYBUFFER_H_*/
