#include "./utils.h"
#include <string>
#include <cstdint>
#include <iostream>
#include <cassert>

#ifndef OCCURRENCE_H_
#define OCCURRENCE_H_

class Occurrence {
 public:
  Occurrence();
  Occurrence(uint64_t _pos, uint64_t _len) : pos(_pos), length(_len), message("") {}
  //Occurrence(uint64_t _pos, string _message) : pos(_pos), message(_message) {}
  Occurrence(string sam_record) : message(sam_record) {
    size_t first_occ = sam_record.find("\t");
    size_t second_occ = sam_record.find("\t", first_occ + 1);
    size_t third_occ = sam_record.find("\t", second_occ + 1);
    size_t fourth_occ = sam_record.find("\t", third_occ + 1);
    size_t fifth_occ = sam_record.find("\t", fourth_occ + 1);
    size_t sixth_occ = sam_record.find("\t", fifth_occ + 1);
    size_t seventh_occ = sam_record.find("\t", sixth_occ + 1);
    size_t eight_occ = sam_record.find("\t", seventh_occ + 1);
    size_t ninth_occ = sam_record.find("\t", eight_occ + 1);
    size_t tenth_occ = sam_record.find("\t", ninth_occ + 1);
    
    string flag_token = sam_record.substr(first_occ + 1, second_occ - first_occ - 1);
    flag = stoi(flag_token);
    //cout << "Flag: " << flag << endl; 

    string pos_token = sam_record.substr(third_occ + 1, fourth_occ - third_occ - 1);
    //cout << "string pos:'" << pos_token << "'" << endl;
    uint64_t one_based_pos = stoul(pos_token);
    //cout << "uint pos:'" << one_based_pos << "'" << endl;
    ASSERT(one_based_pos>0 || flag == 4);
    pos = one_based_pos - 1;
    
    string cigar_token = sam_record.substr(fifth_occ + 1, sixth_occ - fifth_occ - 1);
    //cout << "CIGAR: " << cigar_token << endl;
    this->length = Utils::CigarToLen(cigar_token);
    size_t soft_clipped = Utils::CigarSoftClipped(cigar_token);

    string seq_token = sam_record.substr(ninth_occ + 1, tenth_occ - ninth_occ - 1);
    //cout << "string seq:'" << seq_token << "'" << endl;
    //cout << "strlen(seq)'" << seq_token.size() << "'" << endl;
    
    ASSERT(flag == 4 || seq_token.size()==1||seq_token.size()== length + soft_clipped );
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

  // TODO: Maybe SamRecord instead of message ?
  string GetMessage() {
    return message;
  }
  
  string GetReadName() {
    size_t first_occ = message.find("\t");
    string name = message.substr(0, first_occ);
    return name;
  }

  void UpdatePos(uint64_t new_pos, string new_chr_name = "", int _flag = -1) {
    pos = new_pos;
    if (message != "") {
      size_t first_occ = message.find("\t");
      size_t second_occ = message.find("\t", first_occ + 1);
      size_t third_occ = message.find("\t", second_occ + 1);
      size_t fourth_occ = message.find("\t", third_occ + 1);
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

 private:
  uint64_t pos;
  uint length;
  string message;
  int flag;
};

#endif
