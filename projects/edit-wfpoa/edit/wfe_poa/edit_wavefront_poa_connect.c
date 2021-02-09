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

#include "edit_wavefront_poa_connect.h"

/*
 * Connect wavefront-offset across segments
 */
void edit_wavefront_poa_connect_offset(
    edit_wavefront_poa_t* const wavefront_poa,
    edit_wavefront_segment_t* const wavefront_segment,
    text_dag_t* const text_dag,
    const int distance,
    const int k,
    const ewf_offset_t offset) {
  // Parameters
  text_dag_segment_t* const text_segment = wavefront_segment->text_segment;
  char* const pattern = wavefront_segment->pattern;
  const int pattern_length = wavefront_segment->pattern_length;
  // Check next-connecting segments and open wavefronts
  //   Note that next-segments should be posterior in the partial-ordered graph
  //   Therefore, if connected, the next-segment will be extended on following iterations of this loop
  int i, j;
  for (i=0;i<text_segment->next_total;++i) {
    // Fetch next text-segment
    const int next_idx = text_segment->next[i];
    text_dag_segment_t* const next_text_segment = text_dag->segments_ts[next_idx];
    // Fetch next wavefront-segment
    if (wavefront_poa->wavefront_segments[next_idx] == NULL) {
      wavefront_poa->wavefront_segments[next_idx] = edit_wavefront_segment_new(
          pattern,pattern_length,next_text_segment,wavefront_poa->mm_allocator);
      wavefront_poa->wavefront_segments[next_idx]->index = next_idx;
      wavefront_poa->wavefront_segments[next_idx]->wf_distance_min = distance;
    }
    edit_wavefront_segment_t* const next_wavefront_segment = wavefront_poa->wavefront_segments[next_idx];
    const int next_h = 0; // Changes on g2g
    const int next_v = EWAVEFRONT_V(k,(int)offset);
    const int next_k = EWAVEFRONT_DIAGONAL(next_h,next_v);
    const int next_offset = 0; // Changes on g2g
    // Fetch wavefront
    bool wf_new = false;
    if (next_wavefront_segment->wavefronts[distance] == NULL) {
      next_wavefront_segment->wavefronts[distance] = edit_wavefront_new(
          -pattern_length,next_text_segment->sequence_length,
          next_k,next_k,wavefront_poa->mm_allocator);
      next_wavefront_segment->wf_distance_max = distance;
      wf_new = true;
    }
    edit_wavefront_t* const next_wavefront = next_wavefront_segment->wavefronts[distance];
    // Check current offset
    bool set_offset = false;
    if (!wf_new && next_wavefront->lo <= next_k && next_k <= next_wavefront->hi) {
      if (next_wavefront->offsets[next_k] < next_offset) {
        fprintf(stderr,"DEBUG: Connecting better-offset (I'm interested in seeing this happening)\n"); // TODO
        set_offset = true;
      }
    } else {
      set_offset = true;
    }
    // Set offset
    if (set_offset) {
      next_wavefront->offsets[next_k] = next_offset; // Same k on next-segment
      ++(next_wavefront_segment->num_valid_offsets);
      // Set previous-segment end location
      edit_wavefront_locator_t* const previous_wf_end =
          &next_wavefront_segment->control[next_k].previous_wf_end;
      previous_wf_end->segment_idx = wavefront_segment->index;
      previous_wf_end->distance = distance; // TODO Remove
      previous_wf_end->k = k;
      previous_wf_end->offset = offset;
      // Set current-segment begin location
      edit_wavefront_locator_t* const current_wf_begin =
          &next_wavefront_segment->control[next_k].current_wf_begin;
      current_wf_begin->segment_idx = next_idx;
      current_wf_begin->distance = distance; // TODO Remove
      current_wf_begin->k = next_k;
      current_wf_begin->offset = next_offset;
    }
    // Fill gap in the wavefront (if any)
    if (next_k > next_wavefront->hi) {
      for (j=next_wavefront->hi+1;j<next_k;++j) {
        next_wavefront->offsets[j] = EWAVEFRONT_OFFSET_NULL;
      }
      next_wavefront->hi = next_k;
    } else if (next_k < next_wavefront->lo) {
      for (j=next_k+1;j<next_wavefront->lo;++j) {
        next_wavefront->offsets[j] = EWAVEFRONT_OFFSET_NULL;
      }
      next_wavefront->lo = next_k;
    }
  }
}




