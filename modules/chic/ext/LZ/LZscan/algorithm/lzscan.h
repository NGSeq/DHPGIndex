////////////////////////////////////////////////////////////////////////////////
// lzscan.h
//   The main header for LZscan algorithm. Only this file needs to be included
//   to use LZscan algorithm.
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

#ifndef __PARSE_H
#define __PARSE_H

#include <vector>

// Arguments:
//   X[0..n-1] = input string,
//   b = block size,
//   F = a pointer (can to be NULL) to a container storing the output
//     parsing as a sequence of pairs (pos, len) where pos is a previous
//     phrase occurrence (assuming len > 0) and len is the phrase length.
//     If len = 0, then pos holds the next text symbol.
// Returns:
//   the number of phrases in the parsing of X.
int parse(unsigned char *X, int n, int b, std::vector<std::pair<int, int> > *F);

#endif // __PARSE_H
