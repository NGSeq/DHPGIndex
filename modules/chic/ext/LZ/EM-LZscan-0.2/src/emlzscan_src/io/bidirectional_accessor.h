/**
 * @file    src/emlzscan_src/io/bidirectional_accessor.h
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

#ifndef __EMLZSCAN_SRC_IO_BIDIRECTIONAL_ACCESSOR_H_INCLUDED
#define __EMLZSCAN_SRC_IO_BIDIRECTIONAL_ACCESSOR_H_INCLUDED

#include <cstdio>
#include <cstdint>
#include <string>
#include <algorithm>

#include "../utils.h"


namespace emlzscan_private {

template<typename value_type>
class bidirectional_accessor {
  public:
    bidirectional_accessor(std::string filename, std::uint64_t buf_bytes = (2UL << 20)) {
      m_buf_size = std::max(2UL, buf_bytes / sizeof(value_type));
      m_buf = new value_type[m_buf_size];
      m_file = utils::file_open(filename, "r");
      m_items_total = utils::file_size(filename) / sizeof(value_type);
      m_bytes_read = 0;
      m_origin = 0;
      m_filled = 0;
    }

    inline value_type access(std::uint64_t i) {
      if (i < m_origin || i >= m_origin + m_filled) {

        // Refill buffer centered at position i.
        // (but save as much I/O as possible).
        std::uint64_t neworigin = std::max(0L, (std::int64_t)i - (std::int64_t)m_buf_size / 2);
        if (neworigin < m_origin && neworigin + m_buf_size > m_origin) {

          // Case 1, the suffix of the buffer can be copied.
          std::uint64_t origin_dist = m_origin - neworigin;
          std::uint64_t overlap_length = std::min(m_buf_size - origin_dist, m_filled);
          m_filled = origin_dist + overlap_length;
          m_origin = neworigin;
          for (std::uint64_t j = 0; j < overlap_length; ++j)
            m_buf[m_filled - 1 - j] = m_buf[overlap_length - 1 - j];
          utils::read_at_offset(m_buf, m_origin, origin_dist, m_file);
          m_bytes_read += origin_dist;
        } else if (m_origin <= neworigin && neworigin < m_origin + m_filled) {

          // Case 2, the prefix of the buffer can be copied.
          std::uint64_t origin_dist = neworigin - m_origin;
          std::uint64_t overlap_length = m_filled - origin_dist;
          m_filled = std::min(m_items_total - neworigin, m_buf_size);
          m_origin = neworigin;
          for (std::uint64_t j = 0; j < overlap_length; ++j)
            m_buf[j] = m_buf[origin_dist + j];
          utils::read_at_offset(m_buf + overlap_length, m_origin +
                overlap_length, m_filled - overlap_length, m_file);
          m_bytes_read += m_filled - overlap_length;
        } else {

          // Case 3, whole buffer needs to be read.
          m_origin = neworigin;
          m_filled = std::min(m_buf_size, m_items_total - m_origin);
          utils::read_at_offset(m_buf, m_origin, m_filled, m_file);
          m_bytes_read += m_filled;
        }
      }

      return m_buf[i - m_origin];
    }

    ~bidirectional_accessor() {
      std::fclose(m_file);
      delete[] m_buf;
    }

    inline std::uint64_t bytes_read() const {
      return m_bytes_read;
    }

  private:
    value_type *m_buf;
    std::FILE *m_file;

    std::uint64_t m_bytes_read;
    std::uint64_t m_items_total;
    std::uint64_t m_buf_size;
    std::uint64_t m_origin;
    std::uint64_t m_filled;
};

}  // namespace emlzscan_private

#endif  // __EMLZSCAN_SRC_IO_BIDIRECTIONAL_ACCESSOR_H_INCLUDED
