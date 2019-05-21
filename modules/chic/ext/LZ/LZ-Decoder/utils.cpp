#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "utils.h"

namespace utils {

#define STRX(x) #x
#define STR(x) STRX(x)

void execute(std::string cmd) {
  int system_ret = system(cmd.c_str());
  if (system_ret) {
    fprintf(stderr, "Error: executing command [%s] returned %d.\n",
        cmd.c_str(), system_ret);
    std::exit(EXIT_FAILURE);
  }
}

long double wclock() {
  timeval tim;
  gettimeofday(&tim, NULL);

  return tim.tv_sec + (tim.tv_usec / 1000000.0L);
}

FILE *open_file(std::string fname, std::string mode) {
  FILE *f = std::fopen(fname.c_str(), mode.c_str());
  if (!f) {
    std::perror(fname.c_str());
    std::exit(EXIT_FAILURE);
  }

  return f;
}

long file_size(std::string fname) {
  FILE *f = open_file(fname, "rt");
  std::fseek(f, 0, SEEK_END);
  long size = (long)ftell(f);
  std::fclose(f);

  return size;
}

void file_delete(std::string fname) {
  int res = std::remove(fname.c_str());
  if (res) {
    fprintf(stderr, "Failed to delete %s: %s\n",
        fname.c_str(), strerror(errno));
    std::exit(EXIT_FAILURE);
  }
}

void write_text_to_file(unsigned char *text, long length, std::string fname) {
  FILE *f = open_file(fname, "w");
  long fwrite_ret = std::fwrite(text, sizeof(unsigned char), length, f);
  if (fwrite_ret != length) {
    fprintf(stderr, "Error: fwrite in line %s of %s returned %ld\n",
        STR(__LINE__), STR(__FILE__), fwrite_ret);
    std::exit(EXIT_FAILURE);
  }
  std::fclose(f);
}

void read_block(std::string fname, long beg, long length, unsigned char *b) {
  std::FILE *f = open_file(fname.c_str(), "r");
  std::fseek(f, beg, SEEK_SET);
  read_objects_from_file<unsigned char>(b, length, f);
  std::fclose(f);
}

int random_int(int p, int r) {
  return p + rand() % (r - p + 1);
}

void fill_random_string(unsigned char* &s, long length, int sigma) {
  for (long i = 0; i < length; ++i)
    s[i] = random_int(0, sigma - 1);
}

} // namespace utils
