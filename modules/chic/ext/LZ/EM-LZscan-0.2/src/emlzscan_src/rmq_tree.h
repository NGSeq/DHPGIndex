/**
 * @file    src/emlzscan_src/rmq_tree.h
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

#ifndef __EMLZSCAN_SRC_RMQ_TREE_H_INCLUDED
#define __EMLZSCAN_SRC_RMQ_TREE_H_INCLUDED

#include <algorithm>
#include <vector>
#include <cstdint>


namespace emlzscan_private {

template<typename value_type, std::uint64_t k_block_size_log = 7>
struct rmq_tree {
  private:
    static const std::uint64_t k_block_size;
    static const std::uint64_t k_block_size_mask;
    static const std::uint64_t k_scan_size;

    const value_type *m_tab;
    std::vector<value_type> m_tree;

    std::uint64_t m_length;
    std::uint64_t m_leaves;

  public:
    rmq_tree(const value_type *tab, std::uint64_t length) {
      m_tab = tab;
      m_length = length;

      // Allocate the tree.
      m_leaves = 1;
      while ((m_leaves << k_block_size_log) < length)
        m_leaves <<= 1;
      m_tree.resize(m_leaves << 1, length);

      // Initialize the tree.
      for (std::uint64_t i = 0, j = 0; i < length; i += k_block_size, ++j)
        m_tree[m_leaves + j] = *std::min_element(tab + i, tab + std::min(m_length, i + k_block_size));
      for (std::uint64_t i = m_leaves - 1; i >= 1; --i)
        m_tree[i] = std::min(m_tree[2 * i], m_tree[2 * i + 1]);
    }

    // Given 0 <= i <= m_length, return the largest j such that
    // 0 <= j < i and x[j] < val, or m_length if no such j exists.
    inline std::uint64_t psv(std::uint64_t i, value_type val) const {
      // Scan nearby positions.
      std::uint64_t j = i;
      while (i > 0 && m_tab[i - 1] >= val && j - i < k_scan_size) --i;
      if (i > 0 && m_tab[i - 1] < val) return i - 1;
      else if (i == 0) return m_length;

      // Scan up to a block boundary.
      for (j = i; j & k_block_size_mask; --j)
        if (m_tab[j - 1] < val) return j - 1;

      // Locate the lowest left-neighbour with key < val.
      for (i = m_leaves + (j >> k_block_size_log); i != 1; i >>= 1)
        if ((i & 1) && m_tree[i - 1] < val) { --i; break; }
      if (i == 1) return m_length;

      // Narrow the range to a single block and scan it.
      while (i < m_leaves) i = (i << 1) + (m_tree[2 * i + 1] < val);
      for (i = (i - m_leaves) << k_block_size_log, j = std::min(m_length, i + k_block_size); i < j; --j)
        if (m_tab[j - 1] < val) return j - 1;

      // The smaller element was not found.
      return m_length;
    }

    // Given 0 <= i <= m_length, return the smallest j such that
    // i < j < m_length and x[j] < val, or m_length if no such j exists.
    inline std::uint64_t nsv(std::uint64_t i, value_type val) const {
      // Scan nearby positions.
      std::uint64_t j = i;
      while (i < m_length && m_tab[i] >= val && i - j < k_scan_size) ++i;
      if (i < m_length && m_tab[i] < val) return i;
      else if (i == m_length) return m_length;

      // Scan up to a block boundary.
      for (j = i + 1; j < m_length && (j & k_block_size_mask); ++j)
        if (m_tab[j] < val) return j;
 
      // Locate the lowest right-neighbour with key < val.
      for (i = m_leaves + ((j - 1) >> k_block_size_log); i != 1; i >>= 1)
        if (!(i & 1) && m_tree[i + 1] < val) { ++i; break; }
      if (i == 1) return m_length;

      // Narrow the range to a single block and scan it.
      while (i < m_leaves) i = (i << 1) + (m_tree[2 * i] >= val);
      for (i = (i - m_leaves) << k_block_size_log, j = std::min(m_length, i + k_block_size); i < j; ++i)
        if (m_tab[i] < val) return i;

      // The smaller element was not found.
      return m_length;
    }
};

template<typename value_type, std::uint64_t k_block_size_log>
const std::uint64_t rmq_tree<value_type, k_block_size_log>::k_block_size = (1UL << k_block_size_log);

template<typename value_type, std::uint64_t k_block_size_log>
const std::uint64_t rmq_tree<value_type, k_block_size_log>::k_block_size_mask = (1UL << k_block_size_log) - 1;

template<typename value_type, std::uint64_t k_block_size_log>
const std::uint64_t rmq_tree<value_type, k_block_size_log>::k_scan_size = 512;

}  // namespace emlzpscan_private

#endif  // __EMLZSCAN_SRC_RMQ_TREE_H_INCLUDED
