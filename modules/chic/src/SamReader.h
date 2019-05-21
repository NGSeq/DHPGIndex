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

#ifndef SAM_READER_H_
#define SAM_READER_H_

#include <vector>
#include <string>
#include <ostream>
#include <fstream>
#include "./utils.h"
#include "./occurrence.h"

class SamReader {
  public:
    SamReader();
    SamReader(string _filename, size_t _chunk_size, int _n_threads, int _verbose);

    inline bool IsReady() {
      //return is_ready;
      return my_fstream.eof();
    }
    
    inline void ReadOccurrences(Occurrence * buffer, size_t intended_length, size_t *buffer_len) {
      // it assumes buffer is already allocated, and its lengths is at least intended_length,
      
      if (!my_fstream.good() || !my_fstream.is_open()) {
        cerr << "Error in FetchOccurrences ..." << endl;
        exit(EXIT_FAILURE);
      }
      size_t i = 0;
      while (i < intended_length && getline(my_fstream, buffer[i].message)) {
				// Currently we ignore the headers: 
        if ((buffer[i].message)[0] == '@') continue;  
        i++;
      }
      *buffer_len = i;
    }

    inline vector<Occurrence> FetchOccurrences(size_t n_occs) {
      vector<Occurrence> ans;
      ans.reserve(n_occs);
      if (!my_fstream.good() || !my_fstream.is_open()) {
        cerr << "Error in FetchOccurrences ..." << endl;
        exit(EXIT_FAILURE);
      }
      string line;
      while (ans.size() < n_occs && getline(my_fstream, line)) {
				// Currently we ignore the headers: 
        if (line[0] == '@') continue; 
        ans.push_back(Occurrence(line));
      }
      return ans;
    }
    virtual ~SamReader();

  private:
    int verbose;
    int n_threads;
    size_t chunk_size;
    string filename;
    bool is_ready;

    std::ifstream my_fstream;
};

#endif /* SAM_READER_H_ */
