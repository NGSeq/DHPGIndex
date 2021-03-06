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

#ifndef OCCURRENCE_H_
#define OCCURRENCE_H_
#include "./basic.h"
#include <string>
#include <cstdint>
#include <iostream>


using std::string;

class Occurrence {
 public:
  Occurrence() {
  }
  
	Occurrence(uint64_t _pos, uint64_t _len) : pos(_pos), length(_len), message("") {
  flag = 1;  // anything else than 4 is ok.
  }
  
	Occurrence(string sam_record) : message(sam_record) {
    Init();
  }
	
	~Occurrence() {
  }

  void Init() {
    uint first_occ = message.find("\t");
    uint second_occ = message.find("\t", first_occ + 1);
    uint third_occ = message.find("\t", second_occ + 1);
    uint fourth_occ = message.find("\t", third_occ + 1);
    uint fifth_occ = message.find("\t", fourth_occ + 1);
    uint sixth_occ = message.find("\t", fifth_occ + 1);
    uint seventh_occ = message.find("\t", sixth_occ + 1);
    uint eight_occ = message.find("\t", seventh_occ + 1);
    uint ninth_occ = message.find("\t", eight_occ + 1);
    uint tenth_occ = message.find("\t", ninth_occ + 1);
    
    string flag_token = message.substr(first_occ + 1, second_occ - first_occ - 1);
    flag = stoi(flag_token);

    string pos_token = message.substr(third_occ + 1, fourth_occ - third_occ - 1);
    //cout << "string pos:'" << pos_token << "'";
    //fprintf(stderr, "Pos %s", pos_token);
    uint64_t one_based_pos = stoul(pos_token);
    //cout << "uint pos:'" << one_based_pos << "'";
    ASSERT(one_based_pos>0 || flag == 4);
    pos = one_based_pos - 1;
    
    string cigar_token = message.substr(fifth_occ + 1, sixth_occ - fifth_occ - 1);
    //cout << "CIGAR: " << cigar_token;
    //fprintf(stderr, "CIGAR %s", cigar_token);

    this->length = CigarToLen(cigar_token);
    size_t soft_clipped = CigarSoftClipped(cigar_token);
      //fprintf(stderr, "pos_token %s", pos_token.c_str());


      string seq_token = message.substr(ninth_occ + 1, tenth_occ - ninth_occ - 1);
    //cout << "string seq:'" << seq_token << "'";
    //cout << "strlen(seq)'" << seq_token.size() << "'";
    
    //ASSERT(flag == 4 || seq_token.size()==1||seq_token.size()== length + soft_clipped );
      //fprintf(stderr, "seq token %s", seq_token.c_str());
  }
    void InitBlast() {
        size_t first_occ = message.find("\t");
        size_t second_occ = message.find("\t", first_occ + 1);
        size_t third_occ = message.find("\t", second_occ + 1);
        size_t fourth_occ = message.find("\t", third_occ + 1);
        size_t fifth_occ = message.find("\t", fourth_occ + 1);
        size_t sixth_occ = message.find("\t", fifth_occ + 1);
        size_t seventh_occ = message.find("\t", sixth_occ + 1);
        size_t eight_occ = message.find("\t", seventh_occ + 1);
        size_t ninth_occ = message.find("\t", eight_occ + 1);
        size_t tenth_occ = message.find("\t", ninth_occ + 1);
        size_t eleventh_occ = message.find("\t", tenth_occ + 1);
        size_t tvelvth_occ = message.find("\t", eleventh_occ + 1);

        //string pos_token = message.substr(eight_occ + 1, ninth_occ- eight_occ - 1);
        //string end_token = message.substr(ninth_occ + 1, tenth_occ- ninth_occ - 1);

        uint64_t sstart = stoul(message.substr(eight_occ + 1, ninth_occ- eight_occ - 1));
        uint64_t send = stoul(message.substr(ninth_occ + 1, tenth_occ- ninth_occ - 1));
        uint64_t qseqlen = send-sstart;
        //this->length = qseqlen;
        this->length = stoul(message.substr(third_occ + 1, fourth_occ - third_occ - 1));
        pos=stoul(message.substr(eight_occ + 1, ninth_occ- eight_occ - 1));
        //uint64_t sstart = stoul(message.substr(eight_occ + 1, ninth_occ- eight_occ - 1));
        //uint64_t send = stoul(end_token);
        //uint64_t qseqlen = send-sstart;

        //cout << pos_token << "new pos" <<  new_pos << endl;
        //cout << end_token << "new end" <<  std::to_string(new_pos+1+qseqlen) << endl;

        //NOTE! REPLACE ALWAYS DESCENDING ORDER

    }

  bool IsUnmapped() {
    return (flag == 4);
  }

  uint64_t GetPos() {
    return pos;
  }

  uint GetLength() {
    return length;
  }

  string GetSamRecord() {
    return message;
  }
  
  void SetMessage(string new_message) {
    message = new_message;
  }
  
  string GetReadName() {
    size_t first_occ = message.find("\t");
    string name = message.substr(0, first_occ);
    return name;
  }

  void UpdatePos(uint64_t new_pos, string new_chr_name = "", int _flag = -1) {
    pos = new_pos;
    if (message.length() != 0) {
      uint first_occ = message.find("\t");
      uint second_occ = message.find("\t", first_occ + 1);
      uint third_occ = message.find("\t", second_occ + 1);
      uint fourth_occ = message.find("\t", third_occ + 1);
      string pos_token = message.substr(third_occ + 1, fourth_occ - third_occ - 1);
      message.replace(third_occ + 1, fourth_occ - third_occ - 1, std::to_string(new_pos+1));

      if (new_chr_name.length() != 0) {
        message.replace(second_occ + 1, third_occ - second_occ - 1, new_chr_name);
      }

      if (_flag != -1) {
        flag = _flag;
        message.replace(first_occ + 1, second_occ - first_occ - 1, std::to_string(_flag));
      }
    }
  }
    void UpdatePosBlast(uint64_t new_pos, string new_chr_name = "", int _flag = -1) {
        pos = new_pos;
        if (message != "") {
            size_t first_occ = message.find("\t");
            size_t second_occ = message.find("\t", first_occ + 1);
            size_t third_occ = message.find("\t", second_occ + 1);
            size_t fourth_occ = message.find("\t", third_occ + 1);
            size_t fifth_occ = message.find("\t", fourth_occ + 1);
            size_t sixth_occ = message.find("\t", fifth_occ + 1);
            size_t seventh_occ = message.find("\t", sixth_occ + 1);
            size_t eight_occ = message.find("\t", seventh_occ + 1);
            size_t ninth_occ = message.find("\t", eight_occ + 1);
            size_t tenth_occ = message.find("\t", ninth_occ + 1);
            size_t eleventh_occ = message.find("\t", tenth_occ + 1);
            size_t tvelvth_occ = message.find("\t", eleventh_occ + 1);

            uint64_t sstart = stoul(message.substr(eight_occ + 1, ninth_occ- eight_occ - 1));
            uint64_t send = stoul(message.substr(ninth_occ + 1, tenth_occ- ninth_occ - 1));
            uint64_t qseqlen = send-sstart;

            //cout << pos_token << "new pos" <<  new_pos << endl;
            //cout << end_token << "new end" <<  std::to_string(new_pos+1+qseqlen) << endl;

            //NOTE! REPLACE ALWAYS DESCENDING ORDER

            //Replace seq end
            message.replace(ninth_occ + 1, tenth_occ - ninth_occ - 1, std::to_string(new_pos+qseqlen));

            //Replace seq start
            message.replace(eight_occ + 1, ninth_occ - eight_occ - 1, std::to_string(new_pos));

            if (new_chr_name.length() != 0) {
                //string oldname = message.substr(first_occ + 1, second_occ - first_occ - 1);
                //cout << oldname << "new name" <<  new_chr_name << endl;
                message.replace(first_occ + 1, second_occ - first_occ - 1, new_chr_name);
            }

            /*if (_flag != -1) {
                flag = _flag;
                message.replace(first_occ + 1, second_occ - first_occ - 1, std::to_string(_flag));
            }*/
        }
    }

 private:
  uint64_t pos;
  uint length;
 public:
  string message;
 private:
  int flag;

  size_t CigarToLen(string cigar) {
  if (cigar.size() == 1) {
    ASSERT(cigar[0] == '*');
    return 0;
  }
  size_t total = 0;
  size_t i = 0;
  while (i < cigar.size()) {
    ASSERT(isdigit(cigar[i]));
    size_t start_pos = i;
    while (isdigit(cigar[i])) {
      i++;
    }
    // cout << "Op pos is : "<< i << endl;
    char op = cigar[i];
    // cout << "Op is : "<< op << endl;

    string num_token = cigar.substr(start_pos , i - start_pos);
    // cout << "Num token: "<< num_token << endl;
    if (op == 'M' || op == 'I' || op== 'X' || op== '=' || op == 'X') {
      total += stoul(num_token);
    }
    i++;
  }
  // cout << "Returning: " << total << endl;
  return total;
}

size_t CigarSoftClipped(string cigar) {
  if (cigar.size() == 1) {
    ASSERT(cigar[0] == '*');
    return 0;
  }
  size_t total = 0;
  size_t i = 0;
  while (i < cigar.size()) {
    ASSERT(isdigit(cigar[i]));
    size_t start_pos = i;
    while (isdigit(cigar[i])) {
      i++;
    }
    // cout << "Op pos is : "<< i << endl;
    char op = cigar[i];
    // cout << "Op is : "<< op << endl;

    string num_token = cigar.substr(start_pos , i - start_pos);
    // cout << "Num token: "<< num_token << endl;
    if (op == 'S') {
      total += stoul(num_token);
    }
    i++;
  }
  return total;
}
};

#endif
