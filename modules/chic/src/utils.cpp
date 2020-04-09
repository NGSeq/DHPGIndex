// Copyright Daniel Valenzuela
#include "./utils.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

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
    cout << "Error opening file '" << filename << "' for writing." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}

FILE * OpenWriteOrDie(char * filename) {
  FILE * fp = fopen(filename, "w");
  if (!fp) {
    cout << "Error opening file '" << filename << "' for writing." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}

FILE * OpenReadOrDie(char * filename) {
  FILE * fp = fopen(filename, "r");
  if (!fp) {
    cout << "Error opening file '" << filename << "' for readind." << endl;
    exit(EXIT_FAILURE);
  }
  return fp;
}

FILE * OpenReadOrDie(const char * filename) {
  FILE * fp = fopen(filename, "r");
  if (!fp) {
    cout << "Error opening file '" << filename << "' for readind." << endl;
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
    cout << "Command failed. " << endl;
    exit(-1);
  }
}

void DeleteTmpFile(string filename) {
#ifdef NDEBUG
  char * command_clean = new char[1024];
  sprintf(command_clean, "rm -f %s", filename.c_str());

  printf("We wil call:\n %s\n command.", command_clean);
  if (system(command_clean)) {
    cout << "Command failed. " << endl;
    exit(-1);
  }
#else
  cout << "Debug mode. File ' " << filename << "' will remain in disk" << endl;
#endif
}

size_t CigarToLen(string cigar) {
  if (cigar.size() == 1) {
    ASSERT(cigar[0] == '*');
    return 0;
  }
  size_t total = 0;
  size_t i = 0;
  while (i < cigar.size()) {
    ASSERT(isdigit(cigar[i]));
    size_t start_pos = i;
    while (isdigit(cigar[i])) {
      i++;
    }
    // cout << "Op pos is : "<< i << endl;
    char op = cigar[i];
    // cout << "Op is : "<< op << endl;

    string num_token = cigar.substr(start_pos , i - start_pos);
    // cout << "Num token: "<< num_token << endl;
    if (op == 'M' || op == 'I' || op== 'X' || op== '=' || op == 'X') {
      total += stoul(num_token);
    }
    i++;
  }
  // cout << "Returning: " << total << endl;
  return total;
}

size_t CigarSoftClipped(string cigar) {
  if (cigar.size() == 1) {
    ASSERT(cigar[0] == '*');
    return 0;
  }
  size_t total = 0;
  size_t i = 0;
  while (i < cigar.size()) {
    ASSERT(isdigit(cigar[i]));
    size_t start_pos = i;
    while (isdigit(cigar[i])) {
      i++;
    }
    // cout << "Op pos is : "<< i << endl;
    char op = cigar[i];
    // cout << "Op is : "<< op << endl;

    string num_token = cigar.substr(start_pos , i - start_pos);
    // cout << "Num token: "<< num_token << endl;
    if (op == 'S') {
      total += stoul(num_token);
    }
    i++;
  }
  return total;
}

// TODO: make it propper.
string SamRecordForUnmapped(string read_name) {
  string ans = read_name + "\t4\t*\t0\t0\t*\t*\t*\t0\t0\t*";
  return ans;
}

bool IsBioKernel(KernelType kernel_type) {
  return (kernel_type == KernelType::BWA || kernel_type == KernelType::BOWTIE2 || kernel_type == KernelType::BLAST);
}

bool IsNewLine(uchar c) {
  return (c == '\r' || c == '\n');
}

}  // namespace Utils

