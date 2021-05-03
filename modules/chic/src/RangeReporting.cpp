// Copyright Daniel Valenzuela
#include "./RangeReporting.h"
#include <sdsl/rmq_support.hpp>
#include <sdsl/util.hpp>
#include <tuple>
#include <algorithm>
#include <utility>
#include <vector>
#include "./utils.h"

RangeReporting::RangeReporting() {
}


RangeReporting::RangeReporting(vector<pair<uint64_t, uint64_t> > * lz_phrases,
                               uint context_len,
                               int _verbose) {
  size2SRR = 0;
  verbose = _verbose;
  MergePhrases(lz_phrases, 2*context_len);

  vector<tuple<uint64_t, uint64_t, uint64_t> > grid_phrases;
  grid_phrases = phrases_to_grid(lz_phrases);

  this->n_phrases_grid = grid_phrases.size();
  this->sparse_sample_ratio = SPARSE_DENS;

	EncodeRMQonY(grid_phrases);
  EncodeX(grid_phrases);
  EncodePtr(grid_phrases);
  EncodeLimits(lz_phrases);
  ComputeSize();
}

// TODO(CodeQual)
// Maybe move to LZINDEX, altogether with is_literal vector,
// and pass this vector as parameter for construction. Anyway, this code doesnt quite fit here.
void RangeReporting::MergePhrases(vector<pair<uint64_t, uint64_t> > *lz_phrases,
                                  uint threshold) {
  // need to be checked before merging, otherwise, the answer will always be true.
  bool first_is_literal = false;
  size_t first_len = lz_phrases->at(0).second;
  if (first_len != 0) {
    first_is_literal = true;
  }
  if (verbose >= 3) {
    cerr << "Phrases:" << endl;
    for (size_t i = 0; i < lz_phrases->size(); i++) {
      cerr << "(" << lz_phrases->at(i).first << "," << lz_phrases->at(i).second << ") ";
    }
    cerr << endl;
  }
  is_literal.resize(lz_phrases->size());
  size_t j = 0;
  for (size_t i = 0; i < lz_phrases->size();) {
    uint len = lz_phrases->at(i).second;
    if (len == 0) len = 1;

    if (len > threshold) {
      is_literal[j] = false;
      lz_phrases->at(j) = pair<uint64_t, uint>(lz_phrases->at(i).first, len);
      j++;
      i++;
      continue;
    } else {
      while (i+1 < lz_phrases->size()) {
        uint next_len = lz_phrases->at(i + 1).second;
        if (next_len == 0) next_len = 1;
        if (next_len <= threshold) {
          len += next_len;
          i++;
        } else {
          break;
        }
      }
      // we got a merged phrase to insert:
      lz_phrases->at(j) = pair<uint64_t, uint>(lz_phrases->at(i).first, len);
      is_literal[j] = true;
      j++;
      i++;
    }
  }
  is_literal.resize(j);
  if (first_is_literal) {
    // This enable us to accpet RLZ - Prefix parse.
    // If that was used, the first phrase is a long one (size of ref).
    is_literal[0] = true;
    cerr << "First phrase is long, assuming RLZ." << endl;
    cerr << "Length of first phrase: " << first_len << endl;
  }
  lz_phrases->resize(j);

  size2SRR += size_in_bytes(is_literal);

  if (verbose >= 3) {
    cerr << "Merged phrases:" << endl;
    for (size_t i = 0; i < lz_phrases->size(); i++) {
      if (!is_literal[i]) {
        cerr << "(" << lz_phrases->at(i).first << "," << lz_phrases->at(i).second << ") ";
      } else {
        cerr << "(str," << lz_phrases->at(i).second << ") ";
      }
    }
    cerr << endl;

    cerr << "Is Literal Flag:" << endl;
    for (size_t i = 0; i < lz_phrases->size(); i++) {
      cerr << is_literal[i] << " ";
    }
    cerr << endl;
  }
}

void RangeReporting::EncodeRMQonY(vector<tuple<uint64_t, uint64_t, uint64_t> > grid_phrases) {
  int_vector<> tmp_Y(n_phrases_grid);
  for (size_t i = 0; i < n_phrases_grid; i++) {
    uint64_t x_val = std::get<0>(grid_phrases.at(i));
    uint64_t y_val = std::get<1>(grid_phrases.at(i));
    tmp_Y[i] = x_val + y_val - (uint64_t)1;;
  }

  // false => range MAX queries.
  new_rmq = rmq_succinct_sct<false>(&tmp_Y);
  clear(tmp_Y);
  size2SRR += size_in_bytes(new_rmq);
}

void RangeReporting::EncodeX(vector<tuple<uint64_t, uint64_t, uint64_t> > grid_phrases) {
  uint sparser_sample_X_len = n_phrases_grid/sparse_sample_ratio;
  if (n_phrases_grid%sparse_sample_ratio)
    sparser_sample_X_len++;

  sparser_sample_X = vector<uint64_t>(sparser_sample_X_len);

  int_vector<> tmp_X(n_phrases_grid);
  size_t sample_i = 0;
  for (size_t i = 0; i < n_phrases_grid; i++) {
    uint64_t x_val = std::get<0>(grid_phrases.at(i));
    tmp_X[i] = x_val;
    if (i%sparse_sample_ratio == 0) {
      sparser_sample_X[sample_i] = x_val;
      sample_i++;
    }
  }
  new_X = enc_vector<elias_delta, REGULAR_DENS>(tmp_X);
  ASSERT(n_phrases_grid == new_X.size());


  size2SRR += sparser_sample_X_len*sizeof(uint64_t);
  size2SRR += size_in_bytes(new_X);
}

void RangeReporting::EncodePtr(vector<tuple<uint64_t, uint64_t, uint64_t> > grid_phrases) {
  new_ptr.resize(n_phrases_grid);
  for (size_t i = 0; i < n_phrases_grid; i++) {
    new_ptr[i] = std::get<2>(grid_phrases.at(i));
  }
  sdsl::util::bit_compress(new_ptr);
  size2SRR += size_in_bytes(new_ptr);
}

void RangeReporting::EncodeLimits(vector<pair<uint64_t, uint64_t>> * lz_phrases) {
  int_vector<> limits_gog_tmp(1 + lz_phrases->size());
  uint64_t tmp_pos = 0;
  for (size_t i = 0; i < lz_phrases->size(); i++) {
    limits_gog_tmp[i] = tmp_pos;
    uint64_t b = lz_phrases->at(i).second;
    if (b > 0)
      tmp_pos += b;
    else
      tmp_pos++;
  }
  limits_gog_tmp[lz_phrases->size()] = tmp_pos;

  encoded_limits = enc_vector<elias_delta, REGULAR_DENS>(limits_gog_tmp);
  size2SRR += size_in_bytes(encoded_limits);
  if (verbose >= 3) {
    cerr << "Limits: " << endl;
    for (size_t i = 0; i < encoded_limits.size(); i++) {
      cerr << encoded_limits[i] << " ";
    }
    cerr << endl;
  }
}

vector<tuple<uint64_t, uint64_t, uint64_t> > RangeReporting::phrases_to_grid(vector<pair<uint64_t, uint64_t> > * lz_phrases) {  // NOLINT
  vector<tuple<uint64_t, uint64_t, uint64_t> > ans;
  ans.reserve(lz_phrases->size());
  for (size_t i = 0; i < lz_phrases->size(); i++) {
    uint64_t a = lz_phrases->at(i).first;
    uint64_t b = lz_phrases->at(i).second;

    if (!is_literal[i]) {
      ans.push_back(tuple<uint64_t, uint64_t, uint64_t>(a, b, i));
    }
  }
  std::sort(begin(ans), end(ans));
  if (verbose >= 3) {
    cerr << "tmp grid phrases:" << endl;
    for (size_t i = 0; i < ans.size(); i++) {
      uint64_t a = std::get<0>(ans[i]);
      uint64_t b = std::get<1>(ans[i]);
      uint64_t c = std::get<2>(ans[i]);
      cerr << "( " << a << ", " << b << ", " << c << " )" << endl;
    }
  }
  return ans;
}

// ***********************************
// ACCESSORS
// ***********************************
uint64_t RangeReporting::GetX(uint pos) const {
  return new_X[pos];
}

// retrieve the code 'limit[pos]' from the gap code representation of limit array
uint64_t RangeReporting::GetLimit(uint pos) const {
  return encoded_limits[pos];
}

// return the diff between limit[pos] and [pos-1]
uint64_t RangeReporting::GetLimitDiff(uint pos) const {
  // TODO(MicroOptimization): we could optimize here:
  // By Implementing an sdsl operator that access the delta itself, it might help or not.
  uint64_t diff = GetLimit(pos) - GetLimit(pos-1);
  return diff;
}

uint64_t RangeReporting::GetPtr(uint pos) const {
  return new_ptr[pos];
}

uint RangeReporting::GetSizeBytes() const {
  return size2SRR;
}

uint RangeReporting::GetNPhrasesGrid() const {
  return n_phrases_grid;
}

bool RangeReporting::IsLiteral(uint i) const {
  return is_literal[i];
}


void RangeReporting::DetailedSpaceUssage() const {
  cerr << " RMQ             :" << size_in_bytes(new_rmq) << endl;
  cerr << " Sparse sample X :" << sparser_sample_X.size()*sizeof(uint64_t) << endl;
  cerr << " X               :" << size_in_bytes(new_X) << endl;
  cerr << " Ptr             :" << size_in_bytes(new_ptr) << endl;
  cerr << " Limits          :" << size_in_bytes(encoded_limits) << endl;
  cerr << " IsLiteralFlags :" << size_in_bytes(is_literal) << endl;
}



// ***********************************
// QUERIES
// ***********************************


// it stores in occ[nOcc] all secondary occurrences from the segment T[x..y]
void RangeReporting::queryRR(uint64_t x, uint64_t y, vector<uint64_t> * occs) const {
  if (verbose >= 3) {
    cerr << "Starting RR from x = " << x << ", y = " << y << endl;
  }
  uint64_t pred = searchPred(x);
  uint64_t pred_X = GetX(pred);

  if (pred_X > x) {
    if (verbose >= 1) {
      // may happen that the first  phrase in the grid (sorted by X) has X coordinate, say,
      // 20, and we are quering a match at position x say 10. The search for the
      // predecessor should find the first phrase (as there is no "null" answer), but that
      // first phrase is not really a predecesor.
      // TODO(TEST): make a test that triggeer this case.
      cerr << "For x:" << x << " I'm discarding recursive report" << endl;
      cerr << "Because the X val of predecessor is: " << pred_X << " > " << x << endl;
    }
    return;
  }

  recursiveReport(y, 0, pred, occs);
}

uint64_t RangeReporting::searchPred(uint64_t x) const {
  // First a linear scan in sparser table:
  size_t i;
  uint64_t l, r;
  l = 0;
  r = n_phrases_grid - 1;
  for (i = 0; i < sparser_sample_X.size() && sparser_sample_X[i] <= x; i++) {}
  if (i > 0) {
    if (i < sparser_sample_X.size()) {
      l = (i-1)*sparse_sample_ratio;
      r = i*sparse_sample_ratio-1;
    } else {
      l = (i-1)*sparse_sample_ratio;
      r = n_phrases_grid-1;
    }
  } else {
    if (i < sparser_sample_X.size()) {
      l = 0;
      r = std::min(sparse_sample_ratio, n_phrases_grid) -1;
    }
  }
  // now binary serch bewtween l and r:
  if (r > l) {
    uint64_t m = (l+r)>>1;
    uint64_t Xm = GetX(m);

    while (r-l > 7 && (Xm > x || GetX(m+1) <= x)) {
      if (Xm > x)
        r = m-1;
      else
        l = m+1;
      m = (l+r)>>1;
      Xm = GetX(m);
    }
    // linear scan:
    m = l;
    Xm = GetX(m);
    while (m < r && (Xm > x || GetX(m+1) <= x)) {
      m++;
      Xm = GetX(m);
    }
    return m;
  }
  return l;
}


// It stores in occ[1..nOcc] all the secondary occurrences
// found in the interval Y[l..r] using range maximum queries
void RangeReporting::recursiveReport(uint64_t y,
                                     uint64_t l,
                                     uint64_t r,
                                     vector<uint64_t> * occs) const {
  if (verbose >= 3) {
    cerr << "Recursive report: y = " << y << ", l = " << l << ", r = " << r << endl;
  }
  if (l <=r) {
    uint64_t pos = new_rmq(l, r);
    uint64_t posLim = GetPtr(pos);
    uint64_t diff = GetLimitDiff(posLim+1);

    uint64_t x = GetX(pos);
    uint64_t Yp = x + diff;
    if (Yp > y) {
      occs->push_back(pos);
      if (verbose >= 3) {
        cerr << "Pos = " << pos << "found ! " << endl;
      }
      if (pos)
        recursiveReport(y, l, pos-1, occs);
      recursiveReport(y, pos+1, r, occs);
    }
  }
}

void RangeReporting::ComputeSize() {
  uint count = 0;
  count+=size_in_bytes(new_rmq);
  count+=sparser_sample_X.size()*sizeof(uint64_t);
  count+=size_in_bytes(new_X);
  count+=size_in_bytes(new_ptr);
  count+=size_in_bytes(encoded_limits);
  count+=size_in_bytes(is_literal);
  ASSERT(size2SRR == 0 || size2SRR == count);
  size2SRR = count;
}

void RangeReporting::SetFileNames(char * _prefix) {
  index_prefix.assign(_prefix);
  // ASSERT prefix != "null" ?
  // RR
  rmq_filename = index_prefix + ".rmq";
  x_filename = index_prefix + ".x";
  ptr_filename = index_prefix + ".ptr";
  limits_filename = index_prefix + ".limits";
  sparser_sample_X_filename = index_prefix + ".sparseX";
  is_literal_name = index_prefix + ".is_literal";
  /*
  sprintf(rmq_filename, "%s.rmq", index_prefix);
  sprintf(x_filename, "%s.x", index_prefix);
  sprintf(ptr_filename, "%s.ptr", index_prefix);
  sprintf(limits_filename, "%s.limits", index_prefix);
  sprintf(sparser_sample_X_filename, "%s.sparseX", index_prefix);
  sprintf(is_literal_name, "%s.is_literal", index_prefix);
  */
}

void RangeReporting::Save() const {
  store_to_file(new_rmq, rmq_filename.c_str());
  store_to_file(new_X, x_filename.c_str());
  store_to_file(new_ptr, ptr_filename.c_str());
  store_to_file(encoded_limits, limits_filename.c_str());
  store_to_file(sparser_sample_X, sparser_sample_X_filename.c_str());
  store_to_file(is_literal, is_literal_name.c_str());
}

void RangeReporting::Load(char * _prefix, int _verbose) {
  this->verbose = _verbose;
  SetFileNames(_prefix);

  load_from_file(new_rmq, rmq_filename.c_str());
  load_from_file(new_X, x_filename.c_str());
  load_from_file(new_ptr, ptr_filename.c_str());
  load_from_file(encoded_limits, limits_filename.c_str());
  load_from_file(sparser_sample_X, sparser_sample_X_filename.c_str());
  load_from_file(is_literal, is_literal_name.c_str());

  n_phrases_grid = new_X.size();
  sparse_sample_ratio = SPARSE_DENS;
  size2SRR = 0;
  ComputeSize();
}

RangeReporting::~RangeReporting() {
}
