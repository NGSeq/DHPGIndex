////////////////////////////////////////////////////////////////////////////////
// ms_support.h
//   Implements a class ms_support, that enhances suffix array with
//   additional arrays and lookup tables to enable efficient matching
//   statistics computation.
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Juha Karkkainen, Dominik Kempa and Simon J. Puglisi
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#ifndef __MS_SUPPORT_H
#define __MS_SUPPORT_H

#include <numeric>
#include <algorithm>

#include "lcp.h"

struct Interval {
   int beg;
   int end;
};

#define SUCCLCP(i,c) (lookup[c][((i) / ms_block_size) * 3])
#define PREDLCP(i,c) (lookup[c][((i) / ms_block_size) * 3 + 1])
#define RANK(i,c)    (lookup[c][((i) / ms_block_size) * 3 + 2])
#define ms_block_size 512

struct ms_support {
  ms_support(unsigned char *ax, int *aSA, int an)
      :x(ax), SA(aSA), n(an) {
    blocks = (n + ms_block_size - 1) / ms_block_size;

    LCP = new int[n + 1];
    rank = new int[n + 1];
    BWT = new unsigned char[n + 1];

    // Compute LCP array, use rank to store Phi.
    phi_lcp(x, n, SA, LCP, rank);
    LCP[n] = 0;

    // Compute BWT of x.
    dollar = 0;
    for (int i = 0; i < n; ++i) {
      if (SA[i] == n - 1) last_sa = i;
      if (SA[i]) BWT[i] = x[SA[i] - 1];
      else dollar = i;
    }

    // Compute CPS2 intervals, used by longest_prefix.
    static const int pow16 = (1 << 16);
    int *cc = new int[pow16 + 1];
    for (int i = 0; i < pow16; ++i) intervals[i].beg = -1;
    for (int i = 0; i < pow16; ++i) cc[i] = 0;
    for (int i = 0; i < n - 1; ++i) cc[(x[i] << 8) + x[i + 1]]++;
    for (int i = 0, s = 0; i < pow16; ++i) {
      if (cc[i]) {
        intervals[i].beg = s;
        intervals[i].end = s + cc[i] - 1;
        s += cc[i];
      }
      if (s < n && s == last_sa) ++s;
    }
    delete[] cc;

    // Compute special rank.
    for (int i = 0; i < 256; ++i) C[i] = 0;
    for (int i = 0; i < n; ++i)
      if (i != dollar)
        rank[i] = C[BWT[i]]++;

    // Compute the predecessor/successor lookup tables.
    std::fill(chars, chars + 256, false);
    for (int i = 0; i < 257; ++i) C[i] = 0;
    for (int i = 0; i < n; ++i) {
      ++C[x[i] + 1];
      chars[x[i]] = true;
    }
    std::partial_sum(C, C + 256, C);
    std::fill(lookup, lookup + 256, (int *)NULL);
    for (int i = 0; i < 256; ++i) if (chars[i]) {
      lookup[i] = new int[(blocks + 1) * 3];
      std::fill(lookup[i], lookup[i] + (blocks + 1) * 3, -1);
    }
    for (int i = (blocks - 1) * ms_block_size; i >= 0; i -= ms_block_size) {
      int length = std::min(ms_block_size, n - i - 1), min_lcp = n;
      for (int j = i + 1; j <= i + length; ++j) {
        min_lcp = std::min(min_lcp, LCP[j]);
        if (j != dollar && SUCCLCP(i, BWT[j]) == -1)
          SUCCLCP(i, BWT[j]) = min_lcp;
      }
      for (int j = 0; j < 256; ++j)
        if (chars[j] && i + ms_block_size < n && SUCCLCP(i, j) == -1)
          SUCCLCP(i, j) = std::min(min_lcp, SUCCLCP(i + ms_block_size, j));
    }
    for (int i = 0; i < blocks * ms_block_size; i += ms_block_size) {
      int min_lcp = LCP[i];
      for (int j = i - 1; j >= std::max(0, i - ms_block_size); --j) {
        if (j != dollar && RANK(i,BWT[j]) == -1) {
          PREDLCP(i, BWT[j]) = min_lcp;
          RANK(i, BWT[j]) = rank[j];
        }
        min_lcp = std::min(min_lcp, LCP[j]);
      }
      for (int j = 0; j < 256; ++j)
        if (chars[j] && i && RANK(i,j) == -1) {
          PREDLCP(i, j) = std::min(min_lcp, PREDLCP(i - ms_block_size, j));
          RANK(i, j) = RANK(i - ms_block_size, j);
        }
    }
  }

  // Find the largest j < i s.t. BWT[j] = c. If for m = min(LCP[j+1],..,LCP[i])
  // holds minlcp <= m <= maxlcp, return (rank(BWT,j,c), m) and (-1, -1)
  // otherwise.
  inline std::pair<int, int> pred(int i, unsigned char c, int minlcp,
      int maxlcp) {
    int p = i - 1, lcp = std::min(maxlcp, LCP[i]);
    while (p >= 0 && p % ms_block_size
        && (p == dollar || BWT[p] != c) && lcp >= minlcp)
      lcp = std::min(lcp, LCP[p--]);
    if (p < 0 || lcp < minlcp) return std::make_pair(-1, -1);
    else if (p != dollar && BWT[p] == c) return std::make_pair(rank[p], lcp);
    else {
      lcp = std::min(lcp, chars[c] ? PREDLCP(p, c) : -1);
      return lcp >= minlcp ?
        std::make_pair(chars[c] ? RANK(p, c) : -1, lcp)
      : std::make_pair(-1, -1);
    }
  }

  // Analogous to pred.
  inline std::pair<int, int> succ(int i, unsigned char c, int minlcp,
      int maxlcp) {
    int s = i + 1, lcp = std::min(maxlcp, LCP[s]);
    while (s < n && s % ms_block_size
        && (s == dollar || BWT[s] != c) && lcp >= minlcp)
      lcp = std::min(lcp, LCP[++s]);
    if (s >= n || lcp < minlcp) return std::make_pair(-1, -1);
    else if (s != dollar && BWT[s] == c) return std::make_pair(rank[s], lcp);
    else {
      lcp = std::min(lcp, chars[c] ? SUCCLCP(s, c) : -1);
      return lcp >= minlcp ?
        std::make_pair(chars[c] ? (RANK(s, c) + 1) : 0, lcp)
      : std::make_pair(-1, -1);
    }
  }

  // Let t = x[SA[pos]..SA[pos]+lcp). Update pos and lcp so that
  // x[SA[pos]..SA[pos]+lcp) is the longest prefix of ct occurring in x.
  void extend_left(int &pos, int &lcp, unsigned char c) {
    if (!lcp) pos = 0;
    if (pos != dollar && BWT[pos] == c) {
      pos = C[c] + rank[pos] + (c == x[n - 1]);
      ++lcp;
    } else {
      int newpos = -1, newlcp = 0;
      bool firsthalf = (pos % ms_block_size <= ms_block_size / 2);
      std::pair<int, int> pp = firsthalf ?
          pred(pos, c, 0, lcp)
        : succ(pos, c, 0, lcp);
      if (pp.first != -1) {
        newlcp = pp.second + 1;
        newpos = C[c] + pp.first + (c == x[n - 1]);
      }
      if (newpos == -1 || (newlcp <= lcp && LCP[newpos + firsthalf] >= newlcp)){
        pp = firsthalf ? succ(pos, c, newlcp, lcp) : pred(pos, c, newlcp, lcp);
        if (pp.first != -1 && pp.second + 1 > newlcp) {
          newlcp = pp.second + 1;
          newpos = C[c] + pp.first + (c == x[n - 1]);
        }
      }
      if (newpos != -1) { pos = newpos; lcp = newlcp; }
      else { pos = C[c]; lcp = (C[(int)c + 1] > C[c]); }
    }
  }

  // Find the longest prefix of s occurring in x and return (pos, len)
  // such that s[0..len - 1] == x[SA[pos] .. SA[pos] + len).
  inline std::pair<int, int> longest_prefix(unsigned char *s) {
    int lo = 0, hi = n - 1, curlcp = 0;  
    int code = (s[0] << 8) | s[1];
    if (intervals[code].beg != -1) {
      lo = intervals[code].beg;
      hi = intervals[code].end;
      curlcp = 2;
    }
    while (true) {
      int pc, kn, sr; // 2 x binary search
      for (pc = lo, kn = hi, sr = (pc + kn) / 2; pc < kn; sr = (pc + kn) / 2)
        if (SA[sr] + curlcp < n && x[SA[sr] + curlcp] >= s[curlcp]) kn = sr;
        else pc = sr + 1;
      if (SA[pc] + curlcp >= n || x[SA[pc] + curlcp] != s[curlcp]) break;
      lo = pc;
      for (kn = hi, sr = (pc + kn + 1) / 2; pc < kn; sr = (pc + kn + 1) / 2)
        if (SA[sr] + curlcp < n && x[SA[sr] + curlcp] <= s[curlcp]) pc = sr;
        else kn = sr - 1;
      hi = pc;
      ++curlcp;
    }
    return std::make_pair(lo, curlcp);
  }

  ~ms_support() {
    delete[] BWT;
    delete[] LCP;
    delete[] rank;
    for (int i = 0; i < 256; ++i)
      if (lookup[i]) delete[] lookup[i];
  }

  Interval intervals[1 << 16];

  unsigned char *x, *BWT;
  int C[258], *SA, *LCP, *rank;
  int chars[256];
  int n, dollar, last_sa;
  
  int *lookup[256];
  int blocks;
};

#endif // __MS_SUPPORT_H
