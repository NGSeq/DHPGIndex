/**
 * @file    src/emlzscan_src/compute_lcp.h
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

#ifndef __EMLZSCAN_SRC_COMPUTE_LCP_H_INCLUDED
#define __EMLZSCAN_SRC_COMPUTE_LCP_H_INCLUDED

#include <algorithm>
#include <cstdint>


namespace emlzscan_private {

template<typename saidx_t>
void compute_lcp(const std::uint8_t *text, std::uint64_t text_length,
    const saidx_t *text_sa, saidx_t *text_lcp, saidx_t *temp = NULL) {
  saidx_t *phi = text_lcp;
  saidx_t *plcp = temp;

  // Allocate PLCP if needed.
  if (plcp == NULL)
    plcp = new saidx_t[text_length];

  // Compute phi.
  std::uint64_t undefined_phi_position = text_sa[0];
  for (std::uint64_t i = 1; i < text_length; ++i)
    phi[text_sa[i]] = text_sa[i - 1];

  // Compute plcp.
  std::uint64_t lcp = 0;
  for (std::uint64_t i = 0; i < text_length; ++i) {
    lcp = std::max(0L, (std::int64_t)lcp - 1);

    // Special case.
    if (i == undefined_phi_position) {
      lcp = 0;
      plcp[i] = 0;
      continue;
    }

    // Compute plcp[i].
    std::uint64_t phi_i = phi[i];
    while (i + lcp < text_length && phi_i + lcp < text_length &&
        text[i + lcp] == text[phi_i + lcp]) ++lcp;
    plcp[i] = lcp;
  }

  // Permute plcp into lex order.
  for (std::uint64_t i = 0; i < text_length; ++i)
    text_lcp[i] = plcp[text_sa[i]];

  // Clean up.
  if (temp == NULL)
    delete[] plcp;
}

}  // namespace emlzscan_private

#endif  // __EMLZSCAN_SRC_COMPUTE_LCP_H_INCLUDED
