////////////////////////////////////////////////////////////////////////////////
// lcp.cpp
//   Contruction of LCP array using Phi-algorithm from the paper
//
//   Juha Karkkainen, Giovanni Manzini and Simon J. Puglisi
//   Permuted Longest-Common-Prefix Array
//   In Proc. CPM 2009, LNCS 5577:181-192, 2009.
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

#include <algorithm>

#include "lcp.h"

void phi_lcp(unsigned char *t, int n, int *sa, int *lcp, int *temp) {
  int *phi = lcp, *plcp = temp, l = 0;
  for (int i = 1; i < n; ++i)
    phi[sa[i]] = sa[i-1];
  phi[sa[0]] = -1;
  for (int i = 0; i < n; ++i) {
    int j = phi[i];
    if (j == -1) { plcp[i] = 0; continue; }
    else {
      while (i + l < n && j + l < n && t[i + l] == t[j + l]) ++l;
      plcp[i] = l;
      l = std::max(l - 1, 0);
    }
  }
  for (int i = 0; i < n; ++i)
    lcp[i] = plcp[sa[i]];
}
