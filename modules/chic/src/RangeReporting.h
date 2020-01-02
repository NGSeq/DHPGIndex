#ifndef RANGEREPORTING_H
#define RANGEREPORTING_H

#include "utils.h"
#include <vector>
#include <sdsl/rmq_support.hpp>
#include <sdsl/util.hpp>
#include <sdsl/util.hpp>
#include <sdsl/io.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <sdsl/enc_vector.hpp>
#include <sdsl/coder_elias_delta.hpp>


using sdsl::int_vector;
using sdsl::util::clear;
using sdsl::rmq_succinct_sct;
using sdsl::enc_vector;
using sdsl::coder::elias_delta;
using sdsl::store_to_file;
using sdsl::load_from_file;
using sdsl::size_in_bytes;

// Those params give similar times than original implementation, and keep the size smaller.
#define REGULAR_DENS 4
#define SPARSE_DENS 512

class RangeReporting {
  public:
    // Index construction:
    RangeReporting();
    RangeReporting(vector<pair<uint64_t, uint64_t> > * lz_phrases, uint context_len, int verbose);
    void ComputeSize();
    virtual ~RangeReporting();
    void SetFileNames(char * _prefix);
    void Load(char * _prefix, int _verbose);

    void Save() const;


    // Queries and accessors:
    void queryRR(uint64_t x, uint64_t y, vector<uint64_t> * occs) const;

    uint64_t GetX(uint pos) const;
    uint64_t GetLimit(uint pos) const;
    uint64_t GetLimitDiff(uint pos) const;
    uint64_t GetPtr(uint pos) const;
    uint GetSizeBytes() const;
    uint GetNPhrasesGrid() const;
    bool IsLiteral(uint phrase_id) const;
    void DetailedSpaceUssage() const;

  private:
    // Index construction:
    vector<tuple<uint64_t, uint64_t, uint64_t> > phrases_to_grid(vector<pair<uint64_t, uint64_t> > * lz_phrases);
    // it stores in occ[nOcc] all secondary occurrences from the segment T[x..y]


    void MergePhrases(vector<pair<uint64_t, uint64_t> > * _lz_phrases, uint threshold);
    void EncodeX(vector<tuple<uint64_t, uint64_t, uint64_t> > grid_phrases);
    void EncodePtr(vector<tuple<uint64_t, uint64_t, uint64_t> > grid_phrases);
    void EncodeLimits(vector<pair<uint64_t, uint64_t>> * lz_phrases);

    // AUX FUNCTIONS FOR QUERIES:
    // return the index between X[l,r] that is the predecessor for x, by binary search
    uint64_t searchPred(uint64_t x) const;

    // It stores in occ[1..nOcc] all the secondary occurrences found in the interval Y[l..r] using range maximum queries
    void recursiveReport(uint64_t y, uint64_t l, uint64_t r, vector<uint64_t> * occs) const;

// Variables:

    int verbose;
    uint n_phrases_grid;
    uint sparse_sample_ratio;
    vector<uint64_t> sparser_sample_X;
    enc_vector<elias_delta,REGULAR_DENS> new_X;

    int_vector<> new_ptr;

    enc_vector<elias_delta,REGULAR_DENS> encoded_limits;

    rmq_succinct_sct<false> new_rmq;

    int_vector<1> is_literal;

    uint size2SRR;	// total size of this Two Sided RR data structure


    char index_prefix[200];
    // Own files:
    char rmq_filename[200];
    char x_filename[200];
    char ptr_filename[200];
    char limits_filename[200];
    char sparser_sample_X_filename[200];
    char is_literal_name[200];
};

#endif /* NEWTWOSIDEDRR_H_ */
