#ifndef __STREAM
#define __STREAM

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string>

#include "utils.h"

/********************************* usage ***************************************
stream_reader<int> *sr = new stream_reader<int>("input.txt", 1 << 21);
while (!sr->empty()) {
  int next = sr->read();
}
delete sr;
*******************************************************************************/
template<typename data_type>
struct stream_reader {
  stream_reader(std::string fname, int buf_bytes = (4 << 20))
      : m_bufelems(buf_bytes / sizeof(data_type)) {
    f = utils::open_file(fname, "r");
    buffer = new data_type[m_bufelems];
    if (!buffer) {
      fprintf(stderr, "Error: not enough memory for stream reader\n");
      std::exit(EXIT_FAILURE);
    }
    refill();
  }

  ~stream_reader() {
    delete[] buffer;
    std::fclose(f);
  }

  inline data_type peek() {
    return buffer[pos];
  }

  stream_reader& operator++ () {
    ++pos;
    if (pos == filled) refill();

    return *this;
  }

  inline data_type read() {
    data_type ret = buffer[pos++];
    if (pos == filled) refill();
    
    return ret;
  }

  inline bool empty() {
    return (!filled && !refill());
  }
  
private:
  int m_bufelems, filled, pos;
  data_type *buffer;
  std::FILE *f;

  int refill() {
    filled = std::fread(buffer, sizeof(data_type), m_bufelems, f);
    pos = 0;
    return filled;
  }
};

/********************************* usage ***************************************
stream_writer<int> *sw = new stream_writer<int>("output.txt", 1 << 21);
for (int i = 0; i < n; ++i)
  sw->write(SA[i]);
delete sw;
*******************************************************************************/
template<typename data_type>
struct stream_writer {
  stream_writer(std::string fname, int bufsize = (4 << 20))
      : m_bufelems(bufsize / sizeof(data_type)) {
    f = utils::open_file(fname.c_str(), "w");
    buffer = new data_type[m_bufelems];
    if (!buffer) {
      fprintf(stderr, "Error: allocation error in stream_writer\n");
      std::exit(EXIT_FAILURE);
    }
    filled = 0;
  }

  ~stream_writer() {
    if (filled) flush();
    delete[] buffer;
    fclose(f);
  }

  void write(data_type x) {
    buffer[filled++] = x;
    if (filled == m_bufelems) flush();
  }

private:
  void flush() {
    utils::add_objects_to_file(buffer, filled, f);
    filled = 0;
  }

  std::FILE *f;

  int m_bufelems, filled;
  data_type *buffer;
};

#endif // __STREAM
