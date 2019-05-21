/**
 * @file    src/emlzscan_src/match.h
 * @section LICENCE
 *
 * This file is part of EM-LZscan v0.2
 * See: http://www.cs.helsinki.fi/group/pads/
 *
 * Copyright (C) 2013-2016
 *   Juha Karkkainen <juha.karkkainen (at) cs.helsinki.fi>
 *   Dominik Kempa <dominik.kempa (at) gmail.com>
 *   Simon J. Puglisi <simon.puglisi (at) cs.helsinki.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 **/

#ifndef __EMLZSCAN_SRC_MATCH_H_INCLUDED
#define __EMLZSCAN_SRC_MATCH_H_INCLUDED

#include <cstring>
#include <cstdlib>
#include <algorithm>

#include "io/bidirectional_accessor.h"


namespace emlzscan_private {

// Update MS-factorization of t[0..n - 1) (given
// as s, p) to MS-factorization of t[0..n)
void next(std::uint8_t *t, std::uint64_t &n, std::uint64_t &s, std::uint64_t &p) {
  ++n;
  
  if (n == 1) {
    s = 0;
    p = 1;
    return;
  }

  std::uint64_t i = n - 1;
  std::uint64_t r = (i - s) % p;

  while (i < n) {
    std::uint8_t a = t[s + r];
    std::uint8_t b = t[i];

    if (a > b) {
      p = i - s + 1;
      r = 0;
    } else if (a < b) {
      i -= r;
      s = i;
      p = 1;
      r = 0;
    } else {
      ++r;
      if (r == p)
        r = 0;
    }

    ++i;
  }
}

// Given the MS-factorizarion of t[0..n) return the length of its
// longst border (say, k) and find MS-factorization of t[0..k)
std::uint64_t longest_border(std::uint8_t *t, std::uint64_t n, std::uint64_t &s, std::uint64_t &p) {
  if (n > 0 && 3 * p <= n && !memcmp(t, t + p, s))
    return n - p;

  std::uint64_t i = n / 3 + 1, j = 0;
  while (i < n) {
    while (i + j < n && t[i + j] == t[j])
      next(t, j, s, p);

    if (i + j == n)
      return j;

    if (j > 0 && 3 * p <= j && !memcmp(t, t + p, s)) {
      i += p;
      j -= p;
    } else {
      i += j / 3 + 1;
      j = 0;
    }
  }

  return 0;
}

std::pair<std::uint64_t, std::uint64_t> pattern_matching(std::uint64_t end, std::uint64_t n,
    std::uint8_t *pat, std::uint64_t m, bidirectional_accessor<std::uint8_t> *text_accessor) {
  std::uint64_t len = 0;  // length of record match
  std::uint64_t pos = 0;  // position of record match
  std::uint64_t j = 0;    // length of matching pattern prefix
  std::uint64_t s = 0;    // beginning of maxSuf of matching pattern prefix
  std::uint64_t p = 0;    // shortest period of maxSuf of matching pattern prefix
  std::uint64_t i = 0;    // current index in text

  while (i < end) {
    while (j < m && i + j < n && text_accessor->access(i + j) == pat[j])
      next(pat, j, s, p);

    if (j > len) {
      len = j;
      pos = i;
    }

    std::uint64_t oldj = j;
    j = longest_border(pat, j, s, p);

    if (j == 0) ++i;
    else i += oldj - j;
  }

  return std::make_pair(pos, len);
}

}  // namespace emlzscan_private

#endif  // __EMLZSCAN_SRC_MATCH_H_INCLUDED
