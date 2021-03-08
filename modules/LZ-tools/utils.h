#ifndef __UTILS_H
#define __UTILS_H

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

namespace utils {

#define STRX(x) #x
#define STR(x) STRX(x)

void execute(std::string cmd);
long double wclock();
FILE *open_file(std::string fname, std::string mode);
void file_delete(std::string fname);

template<typename value_type>
void read_objects_from_file(value_type* &tab, long length, std::FILE *f) {
  size_t fread_ret = fread(tab, sizeof(value_type), length, f);
  if (fread_ret != (size_t)length) {
    fprintf(stderr, "Error: fread in line %s of %s returned %ld\n",
        STR(__LINE__), STR(__FILE__), fread_ret);
    std::exit(EXIT_FAILURE);
  }
}

template<typename value_type>
void add_objects_to_file(value_type *tab, long length, FILE *f) {
  long fwrite_ret = fwrite(tab, sizeof(value_type), length, f);
  if (fwrite_ret != length) {
    fprintf(stderr, "Error: fwrite in line %s of %s returned %ld\n",
        STR(__LINE__), STR(__FILE__), fwrite_ret);
    std::exit(EXIT_FAILURE);
  }
}

void read_block(std::string fname, long beg, long length, unsigned char *b);
long file_size(std::string fname);
void write_text_to_file(unsigned char *text, long length, std::string fname);
int random_int(int p, int r);
void fill_random_string(unsigned char* &s, long length, int sigma) ;

} // namespace utils

#endif // __UTILS_H
