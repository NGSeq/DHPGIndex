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

    Occurrence(string blast_record, bool isblast) : message(blast_record) {
        // # Fields: query , subject , % identity, alignment length, mismatches, gap opens, q. start, q. end, s. start, s. end, evalue, bit score
        size_t first_occ = blast_record.find("\t");
        size_t second_occ = blast_record.find("\t", first_occ + 1);
        size_t third_occ = blast_record.find("\t", second_occ + 1);
        size_t fourth_occ = blast_record.find("\t", third_occ + 1);
        size_t fifth_occ = blast_record.find("\t", fourth_occ + 1);
        size_t sixth_occ = blast_record.find("\t", fifth_occ + 1);
        size_t seventh_occ = blast_record.find("\t", sixth_occ + 1);
        size_t eight_occ = blast_record.find("\t", seventh_occ + 1);
        size_t ninth_occ = blast_record.find("\t", eight_occ + 1);
        size_t tenth_occ = blast_record.find("\t", ninth_occ + 1);
        size_t eleventh_occ = blast_record.find("\t", tenth_occ + 1);
        size_t tvelvth_occ = blast_record.find("\t", eleventh_occ + 1);

        //string flag_token = blast_record.substr(first_occ + 1, second_occ - first_occ - 1);
        flag = -1;
        //cout << "Flag: " << flag << endl;

        string pos_token = blast_record.substr(eight_occ + 1, ninth_occ- eight_occ - 1);
        //cout << "string pos:'" << pos_token << "'" << endl;
        uint64_t one_based_pos = stoul(pos_token);
        //cout << "uint pos:'" << one_based_pos << "'" << endl;
        ASSERT(one_based_pos>0 || flag == 4);
        pos = one_based_pos - 1;

        //string cigar_token = sam_record.substr(fifth_occ + 1, sixth_occ - fifth_occ - 1);
        //cout << "CIGAR: " << cigar_token << end;
        string len_token = blast_record.substr(seventh_occ + 1, eight_occ - seventh_occ - 1);
        this->length = stoul(len_token);
        message = blast_record;

        //string seq_token = blast_record.substr(ninth_occ + 1, tenth_occ - ninth_occ - 1);
        //cout << "string seq:'" << seq_token << "'" << endl;
        //cout << "strlen(seq)'" << seq_token.size() << "'" << endl;

        //ASSERT(flag == 4 || identity == length);
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

            string pos_token = message.substr(eight_occ + 1, ninth_occ- eight_occ - 1);
            string end_token = message.substr(ninth_occ + 1, tenth_occ- ninth_occ - 1);

            uint64_t sstart = stoul(message.substr(eight_occ + 1, ninth_occ- eight_occ - 1));
            uint64_t send = stoul(end_token);
            uint64_t qseqlen = send-sstart;

            //cout << pos_token << "new pos" <<  new_pos << endl;
            //cout << end_token << "new end" <<  std::to_string(new_pos+1+qseqlen) << endl;

            //NOTE! REPLACE ALWAYS DESCENDING ORDER

            //Replace seq end
            message.replace(ninth_occ + 1, tenth_occ - ninth_occ - 1, std::to_string(new_pos+1+qseqlen));

            //Replace seq start
            message.replace(eight_occ + 1, ninth_occ - eight_occ - 1, std::to_string(new_pos+1));

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
  string message;
  int flag;
};

#endif
