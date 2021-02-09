/*
 *                             The MIT License
 *
 * Wavefront Alignments Algorithms
 * Copyright (c) 2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 * This file is part of WFPOA.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * PROJECT: Partial Order Alignment Wavefront Alignment (WFPOA)
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "edit_wavefront_poa_extend.h"
#include "edit_wavefront_poa_connect.h"

/*
 * Extend exact-matches of Wavefront-Segment
 */
bool edit_wavefront_poa_segment_extend(
    edit_wavefront_poa_t* const wavefront_poa,
    edit_wavefront_segment_t* const wavefront_segment,
    text_dag_t* const text_dag,
    const int distance,
    edit_wavefront_locator_t* const wf_alignment) {
  // Parameters
  text_dag_segment_t* const text_segment = wavefront_segment->text_segment;
  const char* const pattern = wavefront_segment->pattern;
  const int pattern_length = wavefront_segment->pattern_length;
  const char* const text = text_segment->sequence;
  const int text_length = text_segment->sequence_length;
  // Fetch wavefront
  edit_wavefront_t* const wavefront = wavefront_segment->wavefronts[distance];
  ewf_offset_t* const offsets = wavefront->offsets;
  const int k_min = wavefront->lo;
  const int k_max = wavefront->hi;
  // Extend diagonally each wavefront point
  int k;
  for (k=k_min;k<=k_max;++k) {
    // Check diagonal disabled
    if (wavefront_segment->control[k].disabled) {
      offsets[k] = EWAVEFRONT_OFFSET_NULL;
      continue;
    }
    // Locate offset and extend
    int v = EWAVEFRONT_V(k,offsets[k]);
    int h = EWAVEFRONT_H(k,offsets[k]);
    while (v<pattern_length && h<text_length && pattern[v]==text[h]) {
      ++(offsets[k]);
      ++v;
      ++h;
    }
    // Check for sentinel. Sentinel in text means:
    //   (1) Connect to next-segment
    //   (2) Alignment completed (when pattern sentinel is also found)
    if (text[h] == 'X') {
      // Check next-connecting segments
      if (text_segment->next_total == 0) { // End-of-Graph
        // Check end-of-alignment
        if (pattern[v] == 'Y') {
          wf_alignment->k = k;
          wf_alignment->offset = offsets[k];
          wf_alignment->distance = distance;
          wf_alignment->segment_idx = wavefront_segment->index;
          return true; // End-of-Pattern
        }
      } else {
        // Connect with next-segments and open new wavefronts
        edit_wavefront_poa_connect_offset(wavefront_poa,
            wavefront_segment,text_dag,distance,k,offsets[k]);
      }
      // Close offset in current segment
      offsets[k] = EWAVEFRONT_OFFSET_NULL; // FIXME: I don't really like this (nor I fully understand)
      wavefront_segment->control[k].disabled = true;
      --(wavefront_segment->num_valid_offsets);
    }
  }
  // No End-of-Alignment
  return false;
}

