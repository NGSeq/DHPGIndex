////////////////////////////////////////////////////////////////////////////////
// common.cpp
//   Misc functions.
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

#include <ctime>
#include <cstdio>
#include <cstdlib>

#include <string>
#include <map>

#include "common.h"

static std::map<std::string, clock_t> timers;

void new_timer(std::string name) {
  timers[name] = clock();
}

long double elapsed(std::string name) {
  std::map<std::string, clock_t>::iterator it = timers.find(name);
  if (it == timers.end()) {
    fprintf(stderr, "Error: cannot find timer %s.\n", name.c_str());
    exit(1);
  }
  return ((long double)clock() - it->second) / CLOCKS_PER_SEC;
}

void parse_phrase(unsigned char *x, int n, int i, int psv, int nsv,
    int &pos, int &len) {
  pos = -1;
  len = 0;
  if (nsv != -1 && psv != -1) {
    while (x[psv + len] == x[nsv + len]) ++len;
    if (x[i + len] == x[psv + len]) {
      ++len;
      while (x[i + len] == x[psv + len]) ++len;
      pos = psv;
    } else {
      while (i + len < n && x[i + len] == x[nsv + len]) ++len;
      pos = nsv;
    }
  } else if (psv != -1) {
    while (x[psv + len] == x[i + len]) ++len;
    pos = psv;
  } else if (nsv != -1) {
    while (i + len < n && x[nsv + len] == x[i + len]) ++len;
      pos = nsv;
  }
  if (!len) pos = x[i];
}
