/*
 Daniel Valenzuela
 Simple buffer that offer an interface to go character by character from an array or from a file.
 */
#ifndef MYBUFFERHDFS_H_
#define MYBUFFERHDFS_H_


#include "MyBuffer.h"
#include "./jni/hdfs.h"


class MyBufferHDFS : public MyBuffer{
 public:
  MyBufferHDFS(char * filename) {

      fs = hdfsConnect("node-1", 8020);
      hfile = hdfsOpenFile(fs, filename, O_RDONLY |O_CREAT, 0, 0, 0);
      if(!hfile) {
          cout << "HERE3" << endl;
          fprintf(stderr, "Failed to open %s for writing!\n", filename);
      }
      fprintf(stderr, "Opened  HDSF file %s for reading!\n", filename);
      /*char* buffer = "Hello, World!";
      tSize num_written_bytes = hdfsWrite(fs, writeFile, (void*)buffer, strlen(buffer)+1);
      if (hdfsFlush(fs, writeFile)) {
          fprintf(stderr, "Failed to 'flush' %s\n", writePath);
          exit(-1);
      }
      hdfsCloseFile(fs, writeFile);*/

    //ASSERT();
  }
  void SetPos(size_t j) {
    this->pos = (int64_t) j;
  }
  inline uchar GetChar() {
    /*if(!fs.good()) {
      cout << "A PROBLEM OCCURRED WITH FS!! " << endl;
      exit(EXIT_FAILURE);
    }*/
      //unsigned char *buffer = NULL;
      tSize readBytes = 0;
      uint64_t totalRead = 0;

      unsigned char* buffer = (unsigned char*) malloc(sizeof(unsigned char)*1);

      //cout << pos << " ";
      readBytes = hdfsPread(fs,hfile,pos,(void*)buffer,1);
      if (readBytes > 0) {
          //fprintf(stdout, "READ %d \n", readBytes);

      } else {
          if (readBytes < 0) {
              cout << "HERECC" << endl;
              fprintf(stderr, "hdfsPread failed with error %d \n", errno);
              hdfsCloseFile(fs, hfile);
          }

      }

      //fprintf(stdout, " %c ", buffer[0]);
      this->pos++;
    return buffer[0];
  }
  virtual ~MyBufferHDFS() {
  }
 private:
    hdfsFS fs;
    hdfsFile hfile;
    int64_t pos;
};
#endif /* MYBUFFERHDFS_H_ */
