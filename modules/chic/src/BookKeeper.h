#ifndef BOOK_KEEPER_H_
#define BOOK_KEEPER_H_

#include <vector>
#include <string>
#include <zlib.h>
#include "./utils.h"
#include "./occurrence.h"

class BookKeeper {
  public:
    BookKeeper();
    BookKeeper(char * filename, KernelType kernel_type, bool _verbose);
    BookKeeper(char * filename, KernelType kernel_type, uchar ** seq, size_t * seq_len, bool _verbose);
    void ReadPlain(char * filename, uchar ** seq_ans, size_t * seq_len_ans);
    void ReadFasta(char * filename, uchar ** seq_ans, size_t * seq_len_ans);
    void FilterFasta(char * input_filename);
    void CreateMetaData(char * filename);
    
    virtual ~BookKeeper();

    void SetFileNames(char * index_prefix);
    vector<string> SamHeader();
    void Load(char * index_prefix, int _verbose);
    void Save() const;
    char * GetNewFileName();
    void NormalizeOutput(Occurrence& occ);
    
    char bk_filename[200];


    size_t GetTotalLength() {
      return total_length;
    }

  private:
    bool verbose;
    //string new_input_name;
    size_t n_seqs;
    size_t total_length;
    vector<string> seq_names;
    vector<size_t> seq_lengths;
};

#endif /* BOOK_KEEPER_H_ */
