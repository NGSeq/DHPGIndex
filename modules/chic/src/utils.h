/*
	 Copyright 2017, Daniel Valenzuela <dvalenzu@cs.helsinki.fi>

	 This file is part of CHIC aligner.

	 CHIC aligner is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.

	 CHIC aligner is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with CHIC aligner.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include "./occurrence.h"
#include "./basic.h"
#include <stdlib.h>
#include <sys/resource.h>
#include <iostream>
#include <vector>
#include <string>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::string;
using std::tuple;

namespace Utils {
  long double wclock();
  FILE * OpenWriteOrDie(const char * filename);
  FILE * OpenWriteOrDie(char * filename);
  FILE * OpenReadOrDie(const char * filename);
  FILE * OpenReadOrDie(char * filename);
  size_t GetLength(const char * filename);
  size_t GetLength(char * filename);
  string RandomString();
  void DeleteTmpFile(string filename);
  void DeleteFile(string filename);
  string SamRecordForUnmapped(string read_name);
  bool IsBioKernel(KernelType kernel_type);
  bool IsNewLine(uchar c);
}  // namespace Utils

#endif  // SRC_UTILS_H_

