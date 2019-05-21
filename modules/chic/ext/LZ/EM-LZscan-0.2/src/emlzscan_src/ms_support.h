/**
 * @file    src/emlzscan_src/ms_support.h
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

#ifndef __EMLZSCAN_SRC_MS_SUPPORT_H_INCLUDED
#define __EMLZSCAN_SRC_MS_SUPPORT_H_INCLUDED

#include <numeric>
#include <algorithm>
#include <cstdint>

#include "io/bidirectional_accessor.h"


namespace emlzscan_private {

template<typename value_type>
struct lookup_entry {
  lookup_entry() {
    m_succlcp = -1;
    m_predlcp = -1;
    m_rank = -1;
  }

  value_type m_succlcp;
  value_type m_predlcp;
  value_type m_rank;
} __attribute__((packed));

struct interval {
  interval() {}
  interval(std::int64_t beg, std::int64_t end)
    : m_beg(beg), m_end(end) {}

  std::int64_t m_beg;
  std::int64_t m_end;
};

template<typename saidx_t, std::uint64_t k_block_size = 1024UL>
class ms_support {
  public:
    std::uint64_t m_dollar;  // XXX move this outside the class

  private:
    interval m_intervals[1 << 16];

    std::uint64_t m_count[257];
    bool m_occ_char[256];

    lookup_entry<std::int64_t> **m_lookup;

    const std::uint8_t *m_text;
    std::uint8_t *m_bwt;

    const saidx_t *m_sa;
    const saidx_t *m_lcp;
    saidx_t *m_rank;

    std::uint64_t m_text_length;
    std::uint64_t m_last_sa_pos;
    std::uint64_t n_blocks;

  public:
    ms_support(const std::uint8_t *text, const saidx_t *sa, const saidx_t *lcp, saidx_t *rank, std::uint64_t text_length)
        : m_text(text), m_sa(sa), m_lcp(lcp), m_rank(rank), m_text_length(text_length) {
      m_text_length = text_length;
      n_blocks = (m_text_length + k_block_size - 1) / k_block_size;

      // Allocate space for BWT.
      m_bwt = new std::uint8_t[text_length];

      // Compute BWT of text.
      for (std::uint64_t i = 0; i < text_length; ++i) {
        if ((std::uint64_t)m_sa[i] + 1 == text_length) m_last_sa_pos = i;
        if ((std::uint64_t)m_sa[i] > 0) m_bwt[i] = m_text[m_sa[i] - 1];
        else m_dollar = i;
      }

      // Compute CPS2 intervals.
      {
        static const std::uint64_t pow16 = (1UL << 16);
        std::uint64_t *cc = new std::uint64_t[pow16 + 1];
        for (std::uint64_t i = 0; i < pow16; ++i) m_intervals[i].m_beg = -1;
        for (std::uint64_t i = 0; i < pow16; ++i) cc[i] = 0;
        for (std::uint64_t i = 0; i + 1 < m_text_length; ++i)
          cc[((std::uint64_t)m_text[i] << 8) + (std::uint64_t)m_text[i + 1]]++;
        for (std::uint64_t i = 0, s = 0; i < pow16; ++i) {
          if (cc[i]) {
            m_intervals[i] = interval(s, s + cc[i] - 1);
            s += cc[i];
          }
          if (s < m_text_length && s == m_last_sa_pos) ++s;
        }
        delete[] cc;
      }

      // Compute special rank.
      {
        for (std::uint64_t i = 0; i < 256; ++i) m_count[i] = 0;
        for (std::uint64_t i = 0; i < m_text_length; ++i)
          if (i != m_dollar)
            m_rank[i] = m_count[m_bwt[i]]++;
      }

      // Compute m_count and m_occ_char.
      {
        std::fill(m_occ_char, m_occ_char + 256, false);
        std::fill(m_count, m_count + 257, 0);
        for (std::uint64_t i = 0; i < m_text_length; ++i) {
          ++m_count[(std::uint64_t)m_text[i] + 1];
          m_occ_char[m_text[i]] = true;
        }
        std::partial_sum(m_count, m_count + 256, m_count);
      }

      // Compute lookup table.
      {
        m_lookup = new lookup_entry<std::int64_t>*[256];
        std::fill(m_lookup, m_lookup + 256, (lookup_entry<std::int64_t> *)NULL);
        for (std::uint64_t i = 0; i < 256; ++i)
          if (m_occ_char[i] == true)
            m_lookup[i] = new lookup_entry<std::int64_t>[n_blocks + 1];

        // Scan blocks from last to first.
        for (std::uint64_t block_id = n_blocks; block_id > 0; ) {
          --block_id;
          std::uint64_t block_beg = block_id * k_block_size;
          std::int64_t len = std::min(k_block_size, (std::int64_t)m_text_length - block_beg - 1);
          std::uint64_t min_lcp = m_text_length;

          for (std::int64_t j = (std::int64_t)block_beg + 1; j <= (std::int64_t)block_beg + len; ++j) {
            min_lcp = std::min(min_lcp, (std::uint64_t)m_lcp[j]);
            if (j != (std::int64_t)m_dollar && m_lookup[m_bwt[j]][block_id].m_succlcp == -1)
              m_lookup[m_bwt[j]][block_id].m_succlcp = min_lcp;
          }

          for (std::uint64_t c = 0; c < 256; ++c)
            if (m_occ_char[c] == true && block_beg + k_block_size < m_text_length && m_lookup[c][block_id].m_succlcp == -1)
              m_lookup[c][block_id].m_succlcp = std::min((std::int64_t)min_lcp, (std::int64_t)m_lookup[c][block_id + 1].m_succlcp);
        }

        // Scan blocks from first to last.
        for (std::uint64_t block_id = 0; block_id < n_blocks; ++block_id) {
          std::uint64_t block_beg = block_id * k_block_size;
          std::uint64_t min_lcp = m_lcp[block_beg];

          for (std::int64_t j = (std::int64_t)block_beg - 1; j >= std::max(0L, (std::int64_t)block_beg - (std::int64_t)k_block_size); --j) {
            if (j != (std::int64_t)m_dollar && m_lookup[m_bwt[j]][block_id].m_rank == -1) {
              m_lookup[m_bwt[j]][block_id].m_predlcp = min_lcp;
              m_lookup[m_bwt[j]][block_id].m_rank = m_rank[j];
            }
            min_lcp = std::min(min_lcp, (std::uint64_t)m_lcp[j]);
          }

          for (std::uint64_t c = 0; c < 256; ++c) {
            if (m_occ_char[c] == true && block_beg > 0 && m_lookup[c][block_id].m_rank == -1) {
              m_lookup[c][block_id].m_predlcp = std::min((std::int64_t)min_lcp, (std::int64_t)m_lookup[c][block_id - 1].m_predlcp);
              m_lookup[c][block_id].m_rank = m_lookup[c][block_id - 1].m_rank;
            }
          }
        }
      }
    }

    // Find the largest j < i s.t. BWT[j] = c. If for m = min(LCP[j+1],..,LCP[i])
    // holds minlcp <= m <= maxlcp, return (rank(BWT,j,c), m) and (-1, -1) otherwise.
    inline std::pair<std::int64_t, std::int64_t> pred(std::int64_t i, std::uint8_t c, std::int64_t minlcp, std::int64_t maxlcp) const {
      std::int64_t p = i - 1;
      std::int64_t lcp = std::min((std::int64_t)maxlcp, (std::int64_t)m_lcp[i]);

      while (p >= 0 && (p % k_block_size) && (p == (std::int64_t)m_dollar || m_bwt[p] != c) && lcp >= minlcp)
        lcp = std::min(lcp, (std::int64_t)m_lcp[p--]);

      if (p < 0 || lcp < minlcp) return std::make_pair(-1, -1);
      else if (p != (std::int64_t)m_dollar && m_bwt[p] == c) return std::make_pair((std::int64_t)m_rank[p], (std::int64_t)lcp);
      else {
        lcp = std::min(lcp, m_occ_char[c] ? (std::int64_t)m_lookup[c][p / k_block_size].m_predlcp : -1L);
        return lcp >= minlcp ? std::make_pair(m_occ_char[c] ? (std::int64_t)m_lookup[c][p / k_block_size].m_rank : -1L, (std::int64_t)lcp) : std::make_pair(-1L, -1L);
      }
    }

    // Analogous to pred.
    inline std::pair<std::int64_t, std::int64_t> succ(std::int64_t i, std::uint8_t c, std::int64_t minlcp, std::int64_t maxlcp) const {
      std::int64_t s = i + 1;
      std::int64_t lcp = std::min(maxlcp, (std::int64_t)m_lcp[s]);

      while (s < (std::int64_t)m_text_length && s % k_block_size && (s == (std::int64_t)m_dollar || m_bwt[s] != c) && lcp >= minlcp)
        lcp = std::min(lcp, (std::int64_t)m_lcp[++s]);

      if (s >= (std::int64_t)m_text_length || lcp < minlcp) return std::make_pair(-1, -1);
      else if (s != (std::int64_t)m_dollar && m_bwt[s] == c) return std::make_pair((std::int64_t)m_rank[s], (std::int64_t)lcp);
      else {
        lcp = std::min(lcp, m_occ_char[c] ? (std::int64_t)m_lookup[c][s / k_block_size].m_succlcp : -1L);
        return lcp >= minlcp ? std::make_pair(m_occ_char[c] ? (std::int64_t)(m_lookup[c][s / k_block_size].m_rank + 1) : 0L, (std::int64_t)lcp) : std::make_pair(-1L, -1L);
      }
    }

    // Let t = x[SA[pos]..SA[pos]+lcp). Update pos and lcp so that
    // x[SA[pos]..SA[pos]+lcp) is the longest prefix of ct occurring in x.
    void extend_left(std::uint64_t &pos, std::uint64_t &lcp, std::uint8_t c) const {
      if (lcp == 0) pos = 0;
      if (pos != m_dollar && m_bwt[pos] == c) {
        pos = m_count[c] + m_rank[pos] + (c == m_text[m_text_length - 1]);
        ++lcp;
      } else {
        std::int64_t newpos = -1, newlcp = 0;
        bool firsthalf = (pos % k_block_size <= k_block_size / 2);
        std::pair<std::int64_t, std::int64_t> pp = firsthalf ? pred(pos, c, 0, lcp) : succ(pos, c, 0, lcp);

        if (pp.first != -1) {
          newlcp = pp.second + 1;
          newpos = m_count[c] + pp.first + (c == m_text[m_text_length - 1]);
        }

        if (newpos == -1 || (newlcp <= (std::int64_t)lcp && (std::int64_t)m_lcp[newpos + firsthalf] >= newlcp)) {
          pp = firsthalf ? succ(pos, c, newlcp, lcp) : pred(pos, c, newlcp, lcp);
          if (pp.first != -1 && pp.second + 1 > newlcp) {
            newlcp = pp.second + 1;
            newpos = m_count[c] + pp.first + (c == m_text[m_text_length - 1]);
          }
        }

        if (newpos != -1) {
          pos = newpos;
          lcp = newlcp;
        } else {
          pos = m_count[c];
          lcp = (m_count[(std::uint64_t)c + 1] > m_count[c]);
        }
      }
    }

    // Find the longest prefix of pat occurring in x and return (pos, len)
    // such that pat[0..len - 1] == x[SA[pos] .. SA[pos] + len).
    inline std::pair<std::int64_t, std::int64_t> longest_prefix(std::int64_t i, bidirectional_accessor<std::uint8_t> *pat) {
      std::int64_t lo = 0, hi = m_text_length - 1;
      std::uint64_t curlcp = 0;

      std::int64_t code = ((pat->access(i)) << 8) | (pat->access(i + 1));
      if (m_intervals[code].m_beg != -1) {
        lo = m_intervals[code].m_beg;
        hi = m_intervals[code].m_end;
        curlcp = 2;
      }

      while (true) {
        std::int64_t pc, kn, sr;
        for (pc = lo, kn = hi, sr = (pc + kn) / 2; pc < kn; sr = (pc + kn) / 2)
          if ((std::uint64_t)m_sa[sr] + curlcp < m_text_length && m_text[m_sa[sr] + curlcp] >= pat->access(i + curlcp))
            kn = sr;
          else
            pc = sr + 1;

        if ((std::uint64_t)m_sa[pc] + curlcp >= m_text_length || m_text[m_sa[pc] + curlcp] != pat->access(i + curlcp))
          break;

        lo = pc;
        for (kn = hi, sr = (pc + kn + 1) / 2; pc < kn; sr = (pc + kn + 1) / 2)
          if ((std::uint64_t)m_sa[sr] + curlcp < m_text_length && m_text[m_sa[sr] + curlcp] <= pat->access(i + curlcp))
            pc = sr;
          else
            kn = sr - 1;

        hi = pc;
        ++curlcp;
      }
      return std::make_pair(lo, curlcp);
    }

    ~ms_support() {
      delete[] m_bwt;
      for (std::uint64_t c = 0; c < 256; ++c)
        if (m_lookup[c] != NULL)
          delete[] m_lookup[c];
      delete[] m_lookup;
    }
};

}  // namespace emlzscan_private

#endif  // __EMLZSCAN_SRC_MS_SUPPORT_H_INCLUDED

