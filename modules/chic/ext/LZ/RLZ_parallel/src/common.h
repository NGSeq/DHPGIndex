#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sys/time.h>

using std::cout;
using std::endl;

struct Factor {
  uint64_t pos;
  uint64_t len;
};


struct Reader {
  uint8_t buffer;
  size_t nbits;
  FILE *fp;
  Reader(const char *path) : buffer(0), nbits(0) {
    assert(path);
    fp = fopen(path, "rb");
    assert(fp);
  }
  ~Reader() {
    close();
  }
  void close() {
    if (fp == NULL) return;
    fclose(fp);
    fp = NULL;
  }
  void next() {
    if (1 != fread(&buffer, 1, 1, fp)) {
      printf("ERR");
    }
    nbits = 8;
  }
  size_t read() {
    if (nbits == 0) next();
    size_t bit = buffer & 1;
    buffer >>= 1;
    nbits--;
    return bit;
  }
  size_t read(size_t bits) {
    assert(bits > 0);
    size_t value = 0;
    size_t pos = 0;
    while (pos < bits) {
      value |= (read() << pos);
      pos++;
    }
    return value;
  }
};

struct Writer {
  uint8_t buffer;
  size_t nbits;
  FILE *fp;
  Writer(FILE *stream) : buffer(0), nbits(0), fp(stream) {
    assert(stream);
  }
  Writer(const char *path) : buffer(0), nbits(0) {
    assert(path);
    fp = fopen(path, "wb");
    assert(fp);
  }
  ~Writer() {
    close();
  }
  void close() {
    if (fp == NULL) return;
    flush();
    fclose(fp);
    fp = NULL;
  }
  void flush() {
    if (nbits == 0) return;
    fwrite(&buffer, 1, 1, fp);
    buffer = 0;
    nbits = 0;
  }
  void write(size_t bit) {
    assert(bit < 2);
    if (nbits == 8) flush();
    buffer |= (bit << nbits);
    nbits++;
  }
  void write(size_t value, size_t bits) {
    assert(bits > 0);
    do {
      write(value & 1);
      value >>= 1;
      bits--;
    } while (bits > 0);
  }
};

size_t clog2(size_t v) {
  assert(v > 0);
  return (size_t)ceil(log(v)/log(2));
}

size_t s2b(const char *str)
{
  assert(str);

  char *ptr;

  size_t bytes = strtol(str, &ptr, 10);

  switch(*ptr) {
    case 'g':
    case 'G':
      bytes *= 1000; // Fall through.

    case 'm':
    case 'M':
      bytes *= 1000; // Fall through.

    case 'k':
    case 'K':
      bytes *= 1000; // Fall through.

    case 'b':
    case 'B':
    default:
      break;
  }

  return bytes;
}

uint8_t *load_reference(char *path, size_t reference_length)
{
  FILE *fp = fopen(path, "r");
  assert(fp);

  fseek(fp, 0L, SEEK_END);

  size_t file_real_len = (size_t)ftell(fp);
  if (file_real_len < reference_length) {
    cout << "file real len: " << file_real_len << endl; 
    cout << "reference len: " << reference_length << endl; 
    printf("prefix size give for reference is larger than reference file, aborting.\n");
    exit(33);
  }

  fseek(fp, 0L, SEEK_SET);

  uint8_t *text = new uint8_t[reference_length + 1];

  if ((reference_length) != fread(text, 1, reference_length, fp)) {
    printf("ERR");
  }

  fclose(fp);

  return text;
}


bool is_breakline(char c) {
  if (c == '\r' || c == '\n')  
    return true;
  else 
    return false;
}

bool is_forbidden_in_fasta(char c) {
  if (is_breakline(c)) return true;
  return false;
}

void fasta_to_seq(uint8_t *text, size_t text_len, size_t *seq_len) {
  size_t offset = 0;
  size_t i = 0;
  while (i + offset < text_len) {
    if (text[i+offset] == '>') {
      // cout << "One header skipped from REF" << endl;
      while(i + offset < text_len &&
            !is_breakline(text[i+offset])) { 
        //cout << text[i+offset];
        offset++;
      }
      if (i+offset == text_len) {
        offset--;
        text[i+offset] = '\r'; 
      }
      //cout << "-" << endl;
      continue;
    }
    if (is_forbidden_in_fasta(text[i+offset])) {
      offset++;
      continue;
    }

    if (i + offset < text_len) {
      text[i] = text[i+offset];
      //cout << "T'[" << i << " ] = " << text[i] << endl;
    }
    i++;
  }
  assert(i + offset == text_len);

  *seq_len = text_len - offset;
  return;
}

  /*
size_t propper_offset(char * filename, size_t tentative) {
     FILE *fp = fopen(filename, "r");
     assert(fp);
     fseek(fp, 0L, SEEK_END);
     size_t text_len = (size_t)ftell(fp);

  // Assume header is no longer than, say 200 characters.
  size_t context_len = 200;
  size_t start_pos = (tentative > context_len) ? tentative - context_len : 0;
  size_t end_pos = (tentative + context_len < text_len) ? tentative + context_len : text_len;  // not included

  size_t to_read = end_pos - start_pos;
  fseek(fp, start_pos, SEEK_SET);
  uint8_t *context = new uint8_t[to_read];
  if ((to_read) != fread(context, 1, to_read, fp)) {
  printf("Error reading context to find propper offset");
  exit(33);
  }

  size_t local_t = tentative - start_pos;
  size_t i = local_t;
  while (i != 0) {
  if (is_breakline(context[i])) {
  return tentative;
  }
  if (context[i] == '>') {
  // this is the right position, so the parser should skip this header again.
  return start_pos + i;
  }
  i--;
  }
  // we found nothing, probably a very long line, not likely to be a long header.
  // 
  return tentative;
}
*/

uint8_t *load_reference_fasta(char *path, size_t reference_length, size_t *seq_len)
{
  FILE *fp = fopen(path, "r");
  assert(fp);

  fseek(fp, 0L, SEEK_END);

  size_t file_real_len = (size_t)ftell(fp);
  if (file_real_len < reference_length) {
    cout << "file real len: " << file_real_len << endl; 
    cout << "reference len: " << reference_length << endl; 
    printf("prefix size give for reference is larger than reference file, aborting.\n");
    exit(33);
  }

  fseek(fp, 0L, SEEK_SET);

  uint8_t *text = new uint8_t[reference_length + 1];

  if ((reference_length) != fread(text, 1, reference_length, fp)) {
    printf("ERR");
  }

  fclose(fp);
  fasta_to_seq(text, reference_length, seq_len);
  return text;
}

FILE * open_file(char *path, size_t *n)
{
  FILE *fp = fopen(path, "r");
  assert(fp);

  fseek(fp, 0L, SEEK_END);

  *n = (size_t)ftell(fp);

  fseek(fp, 0L, SEEK_SET);

  return fp;
}

void open_fs(char *path, std::ifstream *fs, size_t *n)
{
  FILE *fp = fopen(path, "r");
  assert(fp);

  fseek(fp, 0L, SEEK_END);

  *n = (size_t)ftell(fp);

  fseek(fp, 0L, SEEK_SET);
  fclose(fp);

  fs->open(path);
  assert(fs->good());
  return;
}

size_t file_size(char *path)
{
  FILE *fp = fopen(path, "r");
  assert(fp);

  fseek(fp, 0L, SEEK_END);

  size_t ans  = (size_t)ftell(fp);

  fseek(fp, 0L, SEEK_SET);
  fclose(fp);

  return ans;
}

long double wclock() {
  timeval tim;
  gettimeofday(&tim, NULL);
  return tim.tv_sec + (tim.tv_usec / 1000000.0L);
}

#endif /* COMMON_H */
