/**
 * @file    src/emlzscan_src/io/backward_stream_reader.h
 * @section LICENCE
 *
 * This file is part of EM-LZscan v0.2
 * See: http://www.cs.helsinki.fi/group/pads/
 *
 * Copyright (C) 2013-2016
 *   Juha Karkkainen <juha.karkkainen (at) cs.helsinki.fi>
 *   Dominik Kempa <dominik.kempa (at) gmail.com>
 *   Simon J. Puglisi <simon.puglisi (at) cs.helsinki.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 **/

#ifndef __EMLZSCAN_SRC_IO_BACKWARD_STREAM_READER_H_INCLUDED
#define __EMLZSCAN_SRC_IO_BACKWARD_STREAM_READER_H_INCLUDED

#include <cstdio>
#include <cstdint>
#include <queue>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "../utils.h"
#include "bidirectional_accessor.h"


namespace emlzscan_private {

template<typename value_type>
class backward_stream_reader {
  public:
    backward_stream_reader(std::string filename, std::uint64_t buf_size_bytes = (2UL << 20)) {
      m_pos = utils::file_size(filename) / sizeof(value_type);
      m_accessor = new bidirectional_accessor<value_type>(filename, buf_size_bytes);
    }

    ~backward_stream_reader() {
      delete m_accessor;
    }

    inline value_type read() {
      value_type result = m_accessor->access(m_pos - 1);
      --m_pos;

      return result;
    }

    inline bool empty() {
      return m_pos == 0;
    }

    inline std::uint64_t bytes_read() const {
      return m_accessor->bytes_read();
    }

  private:
    bidirectional_accessor<value_type> *m_accessor;
    std::uint64_t m_pos;
};

}  // namespace emlzscan_private

#endif  // __EMLZSCAN_SRC_IO_BACKWARD_STREAM_READER_H_INCLUDED
