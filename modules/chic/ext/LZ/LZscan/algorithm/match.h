////////////////////////////////////////////////////////////////////////////////
// match.h
//   String matching routines based on Crochemore's time-space optimal
//   string matching algorithm described in the paper
//
//   Maxime Crochemore: String-Matching on Ordered Alphabets
//   Theoretical Computer Science, vol 92, pages 33-47, 1992.
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

#ifndef __MATCH_H
#define __MATCH_H

#include <cstring>

#include <algorithm>

// Update MS-factorization of t[0..n-1) (variables s,p)
// to MS-factorizatoin of t[0..n)
void next(unsigned char *t, int &n, int &s, int &p) {
  ++n; if (n == 1) { s = 0; p = 1; return; }
  int i = n - 1, r = (i - s) % p;
  while (i < n) {
    char a = t[s + r], b = t[i];
    if (a > b) { p = i - s + 1; r = 0; }
    else if (a < b) { i -= r; s = i; p = 1; r = 0; }
    else { ++r; if (r == p) r = 0; } ++i;
  }
}

// Given the MS-factorizarion of t[0..n) return the length of its
// longst border (say, k) and find MS-factorization of P[0..k)
int longest_border(unsigned char *t, int n, int &s, int&p) {
  if (n > 0 && 3 * p <= n && !memcmp(t, t + p, s)) return n - p;
  int i = n / 3 + 1, j = 0;
  while (i < n) {
    while (i + j < n && t[i + j] == t[j]) next(t, j, s, p);
    if (i + j == n) return j;
    if (j > 0 && 3 * p <= j && !memcmp(t, t + p, s)) { i += p, j -= p; }
    else { i += j / 3 + 1; j = 0; }
  }
  return 0;
}

// Return (i, len) that maximizes len = lcp(x[i..n], x[beg..n]), i < beg.
std::pair<int, int> maxlcp(unsigned char *x, int n, int beg) {
  int len = 0, pos = -1, i = 0, j = 0, s, p;
  while (i < beg) {
    while (beg + j < n && x[i + j] == x[beg + j]) next(x + beg, j, s, p);
    if (j > len) { len = j; pos = i; }
    int oldj = j; j = longest_border(x + beg, j, s, p);
    if (!j) ++i; else i += oldj - j;
  }
  return std::make_pair(pos, len);
}

#endif // __MATCH_H
