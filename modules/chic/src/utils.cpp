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

#include "./utils.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <iostream>  // NOLINT
#include <sstream>
#include <fstream>  // NOLINT
#include <string>
#include <vector>
#include "./occurrence.h"

namespace Utils {

// Wall clock time in seconds
long double wclock() {
  timeval tim;
  gettimeofday(&tim, NULL);

  return tim.tv_sec + (tim.tv_usec / 1000000.0L);
}

FILE * OpenWriteOrDie(const char * filename) {
  FILE * fp = fopen(filename, "w");
  if (!fp) {
    cerr << "Error opening file '" << filename << "' for writing." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}

FILE * OpenWriteOrDie(char * filename) {
  FILE * fp = fopen(filename, "w");
  if (!fp) {
    cerr << "Error opening file '" << filename << "' for writing." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}

FILE * OpenReadOrDie(char * filename) {
  FILE * fp = fopen(filename, "r");
  if (!fp) {
    cerr << "Error opening file '" << filename << "' for readind." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}

FILE * OpenReadOrDie(const char * filename) {
  FILE * fp = fopen(filename, "r");
  if (!fp) {
    cerr << "Error opening file '" << filename << "' for readind." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}


string RandomString() {
  srand(time(NULL));
  uint64_t hash = (uint64_t)rand() * RAND_MAX + (uint64_t)rand();
  std::stringstream ss;
  ss << hash;
  return ss.str();
}

size_t GetLength(char * filename) {
  FILE *infile = OpenReadOrDie(filename);
  fseek(infile, 0, SEEK_END);
  size_t len  = (size_t)ftell(infile);
  fseek(infile, 0, SEEK_SET);
  fclose(infile);
  return len;
}

size_t GetLength(const char * filename) {
  FILE *infile = OpenReadOrDie(filename);
  fseek(infile, 0, SEEK_END);
  size_t len  = (size_t)ftell(infile);
  fseek(infile, 0, SEEK_SET);
  fclose(infile);
  return len;
}

void DeleteFile(string filename) {
  char * command_clean = new char[1024];
  sprintf(command_clean, "rm -f %s", filename.c_str());

  printf("We wil call:\n %s\n command.", command_clean);
  if (system(command_clean)) {
    cerr << "Command failed. " << endl;
    exit(-1);
  }
}

void DeleteTmpFile(string filename) {
#ifdef NDEBUG
  char * command_clean = new char[1024];
  sprintf(command_clean, "rm -f %s", filename.c_str());

  printf("We wil call:\n %s\n command.", command_clean);
  if (system(command_clean)) {
    cerr << "Command failed. " << endl;
    exit(-1);
  }
#else
  cerr << "Debug mode. File ' " << filename << "' will remain in disk" << endl;
#endif
}

string SamRecordForUnmapped(string read_name) {
  string ans = read_name + "\t4\t*\t0\t0\t*\t*\t*\t0\t0\t*";
  return ans;
}

bool IsBioKernel(KernelType kernel_type) {
  return (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2);
}

bool IsNewLine(uchar c) {
  return (c == '\r' || c == '\n');
}

}  // namespace Utils

