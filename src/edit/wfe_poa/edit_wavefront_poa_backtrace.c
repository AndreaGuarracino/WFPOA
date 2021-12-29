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

#include "edit_wavefront_poa_backtrace.h"

/*
 * Backtrace Wavefront-POA
 */
void edit_wavefront_poa_backtrace_segment(
    edit_wavefront_segment_t* const wavefront_segment,
    edit_wavefront_locator_t* const wf_loc,
    cigar_t* const cigar) {
  // Parameters wavefront
  edit_wavefront_locator_t wf_init = {
      .segment_idx = 0,
      .distance = 0,
      .k = 0,
      .offset = 0,
  };
  edit_wavefront_control_t* const control = wavefront_segment->control;
  int distance = wf_loc->distance;
  int k = wf_loc->k;
  int offset = wf_loc->offset;
  edit_wavefront_locator_t* wf_begin = (wf_loc->segment_idx!=0) ? &(control[k].current_wf_begin) : &wf_init;
  // Parameters CIGAR
  char* const cigar_operations = cigar->operations;
  int cigar_offset = cigar->begin_offset;
  // Backtrace
  while (wf_begin->distance!=distance || wf_begin->k!=k) {
    // Fetch
    const edit_wavefront_t* const wavefront = wavefront_segment->wavefronts[distance-1];
    const ewf_offset_t* const offsets = wavefront->offsets;
    // Traceback operation
    const ewf_offset_t offset_del =
        (wavefront->lo <= k+1 && k+1 <= wavefront->hi) ? offsets[k+1] : -1;
    const ewf_offset_t offset_ins =
        (wavefront->lo <= k-1 && k-1 <= wavefront->hi) ? (offsets[k-1]+1) : -1;
    const ewf_offset_t offset_mism =
        (wavefront->lo <= k && k <= wavefront->hi) ? (offsets[k]+1) : -1;
    const ewf_offset_t offset_max = MAX(MAX(offset_del,offset_ins),offset_mism);
    // Add matches
    const int num_matches = offset - offset_max;
    int i;
    for (i=0;i<num_matches;++i) {
      cigar_operations[--cigar_offset] = 'M';
    }
    // Add operation
    offset = offset_max;
    if (offset_max == offset_del) {
      cigar_operations[--cigar_offset] = 'D';
      ++k;
      --distance;
    } else if (offset_max == offset_ins) {
      cigar_operations[--cigar_offset] = 'I';
      --k;
      --offset;
      --distance;
    } else { // offset_max == offset_mism
      cigar_operations[--cigar_offset] = 'X';
      --distance;
      --offset;
    }
    // Reload begin-location
    wf_begin = (wf_loc->segment_idx!=0) ? &(control[k].current_wf_begin) : &wf_init;
  }
  // Account for last run of matches
  const int leading_matches = offset - wf_begin->offset;
  int i;
  for (i=0;i<leading_matches;++i) {
    cigar_operations[--cigar_offset] = 'M';
  }
  // Return wf-location (previous segment)
  *wf_loc = control[k].previous_wf_end;
  // Close CIGAR
  cigar->begin_offset = cigar_offset;
}
void edit_wavefront_poa_backtrace(
    edit_wavefront_poa_t* const wavefront_poa,
    edit_wavefront_locator_t* const wf_alignment,
    cigar_t* const cigar) {
  // Parameters
  edit_wavefront_segment_t** const wavefront_segments = wavefront_poa->wavefront_segments;
  // Clear CIGAR
  cigar_clear(cigar);
  // Backtrace from alignment-segment back to the beginning of the text-DAG
  edit_wavefront_locator_t wf_loc = *wf_alignment;
  int segment_idx;
  do {
    // Backtrace segment-region
    segment_idx = wf_loc.segment_idx;
    edit_wavefront_poa_backtrace_segment(wavefront_segments[segment_idx],&wf_loc,cigar);
    // Add segment-idx to CIGAR
    cigar_add_segment(cigar,segment_idx);
  } while (segment_idx > 0);
}







