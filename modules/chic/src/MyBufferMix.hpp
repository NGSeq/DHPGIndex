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

#ifndef MYBUFFERMIX_H_ 
#define MYBUFFERMIX_H_ 

#include "MyBuffer.h"

class MyBufferMix : public MyBuffer{
  public:
    MyBufferMix(char * filename) {
      fs.open(filename);
      ASSERT(fs.good());
      seq = NULL;
    }
    MyBufferMix(uchar * _seq, size_t _len) {
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
          cerr << "A PROBLEM OCCURRED WITH FS!! " << endl;
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
