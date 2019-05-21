////////////////////////////////////////////////////////////////////////////////
// kkp.cpp
//   Implementation of main parsing functions.
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
#ifndef KKP_H
#define KKP_H 

#include <stdint.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>

#include <vector>
#include <algorithm>


#define STACK_BITS 16
#define STACK_SIZE (1 << STACK_BITS)
#define STACK_HALF (1 << (STACK_BITS - 1))
#define STACK_MASK ((STACK_SIZE) - 1)

// An auxiliary routine used during parsing.
int64_t parse_phrase(unsigned char *X, int64_t n, int64_t i, int64_t psv, int64_t nsv,
    std::vector<std::pair<int64_t, int64_t> > *F);

// TODO: current version overwrites SA, this can
// be avoided, similarly as in KKP2.
int64_t kkp3(unsigned char *X, int64_t *SA, int64_t n,
   std::vector<std::pair<int64_t, int64_t> > *F) {
  if (n == 0) return 0;
  int64_t *CPSS = new int64_t[2 * n + 5];
  
  // This can be avoided too.
  for (int64_t i = (int64_t)n; i; --i)
    SA[i] = SA[i - 1];
  SA[0] = SA[n + 1] = -1;

  // Compute PSV_text and NSV_text for SA.
  int64_t top = 0;
  for (int64_t i = 1; i <= n + 1; ++i) {
    while (SA[top] > SA[i]) {
      int64_t addr = (SA[top] << 1);
      CPSS[addr] = SA[top - 1];
      CPSS[addr + 1] = SA[i];
      --top;
    }
    SA[++top] = SA[i];
  }

  // Compute the phrases.
  if (F) F->push_back(std::make_pair(X[0], 0));
  int64_t i = 1, nfactors = 1;
  while(i < n) {
    int64_t addr = (i << 1);
    int64_t psv = CPSS[addr];
    int64_t nsv = CPSS[addr + 1];
    i = parse_phrase(X, n, i, psv, nsv, F);
    ++nfactors;
  }
   
  // Clean up.
  delete[] CPSS;
  return nfactors;
}

int64_t parse_phrase(unsigned char *X, int64_t n, int64_t i, int64_t psv, int64_t nsv,
    std::vector<std::pair<int64_t, int64_t> > *F) {
  int64_t pos, len = 0;
  if (nsv == -1) {
    while (X[psv + len] == X[i + len]) ++len;
    pos = psv;
  } else if (psv == -1) {
    while (i + len < n && X[nsv + len] == X[i + len]) ++len;
    pos = nsv;
  } else {
    while (X[psv + len] == X[nsv + len]) ++len;
    if (X[i + len] == X[psv + len]) {
      ++len;
      while (X[i + len] == X[psv + len]) ++len;
      pos = psv;
    } else {
      while (i + len < n && X[i + len] == X[nsv + len]) ++len;
      pos = nsv;
    }
  }
  if (len == 0) pos = X[i];
  if (len > UINT32_MAX) {
    std::cerr << "A very long phrase larger than UINT32_MAX)" << std::endl;
    std::cerr << "Length: " << len << std::endl;
    std::cerr << "Length(GB): " << (len*1.0)/(1000000000.0) << std::endl;
  }
  
  if (F) F->push_back(std::make_pair(pos, len));
  return i + std::max((int64_t)1, len);
}
#endif /* PARSEREF_H*/
