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

#include "edit_wavefront_poa.h"
#include "edit_wavefront_poa_connect.h"
#include "edit_wavefront_poa_extend.h"
#include "edit_wavefront_poa_display.h"
#include "edit_wavefront_poa_backtrace.h"
#include "alignment/cigar.h"

/*
 * Wavefront-POA compute next wavefront
 */
void edit_wavefront_segment_compute_next(
    edit_wavefront_segment_t* const wavefront_segment,
    const int distance) {
  // Fetch previous wavefront
  edit_wavefront_t* const wavefront = wavefront_segment->wavefronts[distance-1];
  if (wavefront_segment->num_valid_offsets == 0) return; // Check wavefront active
  // Fetch current wavefront
  const int hi = wavefront->hi;
  const int lo = wavefront->lo;
  edit_wavefront_t* const next_wavefront =
      edit_wavefront_new(-wavefront_segment->pattern_length,
          wavefront_segment->text_segment->sequence_length,
          lo-1,hi+1,wavefront_segment->mm_allocator); // TODO Proper boundary check on new WF
  wavefront_segment->wavefronts[distance] = next_wavefront;
  wavefront_segment->wf_distance_max = distance;
  // Fetch offsets
  ewf_offset_t* const offsets = wavefront->offsets;
  ewf_offset_t* const next_offsets = next_wavefront->offsets;
  // Loop peeling (k=lo-1)
  if (offsets[lo] < 0) {
    // next_offsets[lo-1] = EWAVEFRONT_OFFSET_NULL;
    // wavefront_segment->control[lo-1].disabled = true;
    next_wavefront->lo = lo;
  } else {
    next_offsets[lo-1] = offsets[lo];
    ++(wavefront_segment->num_valid_offsets);
  }
  // Loop peeling (k=lo)
  const ewf_offset_t bottom_upper_del = ((lo+1) <= hi) ? offsets[lo+1] : EWAVEFRONT_OFFSET_NULL;
  next_offsets[lo] = MAX(offsets[lo]+1,bottom_upper_del);
  // Compute next wavefront starting point
  int k;
  #pragma GCC ivdep
  for (k=lo+1;k<=hi-1;++k) {
    /*
     * const int del = offsets[k+1]; // Upper
     * const int sub = offsets[k] + 1; // Mid
     * const int ins = offsets[k-1] + 1; // Lower
     * next_offsets[k] = MAX(sub,ins,del); // MAX
     */
    const ewf_offset_t max_ins_sub = MAX(offsets[k],offsets[k-1]) + 1;
    next_offsets[k] = MAX(max_ins_sub,offsets[k+1]);
  }
  // Loop peeling (k=hi)
  const ewf_offset_t top_lower_ins = (lo <= (hi-1)) ? offsets[hi-1] : EWAVEFRONT_OFFSET_NULL;
  next_offsets[hi] = MAX(offsets[hi],top_lower_ins) + 1;
  // Loop peeling (k=hi+1)
  if (offsets[hi]+1 < 0) {
    // next_offsets[hi+1] = EWAVEFRONT_OFFSET_NULL;
    // wavefront_segment->control[hi+1].disabled = true;
    next_wavefront->hi = hi;
  } else {
    next_offsets[hi+1] = offsets[hi] + 1;
    ++(wavefront_segment->num_valid_offsets);
  }
}
/*
 * Wavefront-POA edit distance
 */
void edit_wavefront_poa_align_init(
    edit_wavefront_poa_t* const wavefront_poa,
    char* const pattern,
    const int pattern_length,
    text_dag_t* const text_dag) {
  // Fetch first segment
  text_dag_segment_t* const segment = text_dag->segments_ts[0];
  // Set initial wavefront-segment
  edit_wavefront_segment_t* const wavefront_segment = edit_wavefront_segment_new(
      pattern,pattern_length,segment,wavefront_poa->mm_allocator);
  wavefront_segment->index = 0;
  wavefront_poa->wavefront_segments[0] = wavefront_segment;
  wavefront_segment->wf_distance_min = 0;
  wavefront_segment->wf_distance_max = 0;
  // Set initial wavefront
  edit_wavefront_t* const wavefront = edit_wavefront_new(
      -pattern_length,segment->sequence_length,0,0,wavefront_poa->mm_allocator);
  wavefront_segment->wavefronts[0] = wavefront;
  // Set initial offset
  wavefront->offsets[0] = 0;
  wavefront_segment->num_valid_offsets = 1;
}
void edit_wavefront_poa_align(
    edit_wavefront_poa_t* const wavefront_poa,
    char* const pattern,
    const int pattern_length,
    text_dag_t* const text_dag,
    cigar_t* const cigar) {
  // Parameters
  const int segments_total = text_dag->segments_total;
  edit_wavefront_segment_t** const wavefront_segments = wavefront_poa->wavefront_segments;
  // Set initial wavefront-segment
  edit_wavefront_poa_align_init(wavefront_poa,pattern,pattern_length,text_dag);
  // Compute wavefronts for increasing distance (across wavefront-segment)
  edit_wavefront_locator_t wf_alignment;
  bool aligned = false;
  int distance = 0;
  while (!aligned) {
    // Check all active segments
    int segment_idx;
    for (segment_idx=0;segment_idx<segments_total;++segment_idx) {
      edit_wavefront_segment_t* const wavefront_segment = wavefront_segments[segment_idx];
      if (edit_wavefront_segment_is_active(wavefront_segment,distance)) { // Check active
        // Extend diagonally each wavefront point
        const bool alignment_end = edit_wavefront_poa_segment_extend(
            wavefront_poa,wavefront_segment,text_dag,distance,&wf_alignment);
        // Check exit condition
        if (alignment_end) {
          // DEBUG: To display the WFA
          // edit_wavefront_poa_print(stderr,wavefront_poa,text_dag,distance);
          aligned = true;
          break;
        }
        // Compute next wavefront starting point
        edit_wavefront_segment_compute_next(wavefront_segment,distance+1);
      }
    }
    // Increase distance
    ++distance;
  }
  // Backtrace wavefronts
  edit_wavefront_poa_backtrace(wavefront_poa,&wf_alignment,cigar);
}

