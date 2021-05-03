////////////////////////////////////////////////////////////////////////////////
// rmq_tree.h
//   A heap-type tree holding minima in inner nodes. Built over smallest
//   elements in fixed size blocks of input sequence. Implements NSV and
//   PSV queries: given a sequence of integers A[0..n-1],
//     PSV(i) = max{ j: 0 <= j < i and A[j] < A[i] },
//     NSV(i) = min{ j: i < j < n and A[j] < A[i] }.
//   In either case, if no such j exists, we define the value to be -1.
//
//   The data structure answers the queries in O(log n) time and
//   requires at most 16n/(2^bits) bytes, where bits is a parameter
//   defined in the class. It is a modified version of the data structure
//   described in the paper:
//     R. Canovas & G. Navarro, "Practical Compressed Suffix Trees",
//     In Proc. SEA 2010, LNCS 6049:94-105, 2010.
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


#ifndef __RMQ_TREE_H
#define __RMQ_TREE_H

#include <algorithm>
#include <vector>

struct rmq_tree {
  rmq_tree(int *ax, int an, int b)
      :x(ax), n(an), size(1), bits(b), pow(1 << bits), mask(pow - 1) {
    while ((size << bits) < n) size <<= 1;
    tree.resize(size << 1 , n);

    // Initialize the tree.
    for (int i = 0, j = 0; i < n; i += pow, ++j)
      tree[size + j] = *std::min_element(x + i, x + std::min(n, i + pow));
    for (int i = size - 1; i >= 1; --i)
      tree[i] = std::min(tree[2 * i], tree[2 * i + 1]);
  }

  // Return largest j < i such that x[j] < ub.
  inline int psv(int i, int ub) {
    // Scan nearby positions.
    int j = i;
    while (i >= 0 && x[i] >= ub && j - i < 512) --i;
    if (i >= 0 && x[i] < ub) return x[i];
    else if (i <= 0) return -1;

    // Scan up to a block boundary.
    for (j = i - 1; (j + 1) & mask; --j)
      if (x[j] < ub) return x[j];

    // Locate the lowest left-neighbor with key < ub.
    for (i = size + (i >> bits); i != 1; i >>= 1)
      if ((i & 1) && tree[i - 1] < ub) { --i; break; }
    if (i == 1) return -1;

    // Narrow the range to a single block and scan it.
    while (i < size) i = (i << 1) + (tree[2 * i + 1] < ub);
    for (i = (i - size) << bits, j = std::min(n, i + pow) - 1; i <= j; --j)
      if (x[j] < ub) return x[j];
    return -1;
  }

  // Analogous to psv.
  inline int nsv(int i, int ub) {
    int j = i;
    while (i < n && x[i] >= ub && i - j < 512) ++i;
    if (i < n && x[i] < ub) return x[i];
    else if (i >= n) return -1;

    for (j = i + 1; j < n && (j & mask); ++j)
      if (x[j] < ub) return x[j];
 
    for (i = size + (i >> bits); i != 1; i >>= 1)
      if (!(i & 1) && tree[i + 1] < ub) { ++i; break; }
    if (i == 1) return -1;

    while (i < size) i = (i << 1) + (tree[2 * i] >= ub);
    for (i = (i - size) << bits, j = std::min(n, i + pow); i < j; ++i)
      if (x[i] < ub) return x[i];
    return -1;
  }

  std::vector<int> tree;
  int *x, n, size, bits;
  const int pow, mask;
};

#endif // __RMQ_TREE_H
