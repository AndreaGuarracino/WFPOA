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

#include "edit_wavefront_poa_display.h"

/*
 * Display
 */
void edit_wavefront_poa_print_wavefront_segment(
    FILE* const stream,
    edit_wavefront_segment_t* const wavefront_segment) {
  // Headers
  int k, s;
  fprintf(stream,"[WF.Segment=%d] (Active-offsets=%d)\n",
      wavefront_segment->index,wavefront_segment->num_valid_offsets);
  fprintf(stream,">Pattern=%.*s\n",
      wavefront_segment->pattern_length,
      wavefront_segment->pattern);
  fprintf(stream,">Text=%.*s\n",
      wavefront_segment->text_segment->sequence_length,
      wavefront_segment->text_segment->sequence);
  // Parameters
  const int max_distance = wavefront_segment->wf_distance_max;
  const int min_distance = wavefront_segment->wf_distance_min;
  const int max_k =  (wavefront_segment->text_segment->sequence_length - 1);
  const int min_k = -(wavefront_segment->pattern_length - 1);
  // Traverse all diagonals
  for (k=max_k;k>=min_k;k--) {
    fprintf(stream,"[%s]", (wavefront_segment->control[k]).disabled ? "  ":"ON");
    fprintf(stream,"[k=%3d] ",k);
    // Traverse all scores
    for (s=min_distance;s<=max_distance;++s) {
      edit_wavefront_t* const wavefront = wavefront_segment->wavefronts[s];
      if (wavefront == NULL) {
        fprintf(stream,"      ");
      } else {
        ewf_offset_t* const offsets = wavefront->offsets;
        // Check limits
        const int hi = wavefront->hi;
        const int lo = wavefront->lo;
        if (lo <= k && k <= hi) {
          fprintf(stream,"[%3d] ",offsets[k]);
        } else {
          fprintf(stream,"      ");
        }
      }
    }
    fprintf(stream,"\n");
  }
  // Headers
  fprintf(stream,"SCORE       ");
  for (s=min_distance;s<=max_distance;++s) {
    fprintf(stream," %3d  ",s);
  }
  fprintf(stream,"\n");
}
void edit_wavefront_poa_print(
    FILE* const stream,
    edit_wavefront_poa_t* const wavefront_poa,
    text_dag_t* const text_dag,
    const int distance) {
  fprintf(stream,"--------------------------------------------------------------------------------\n");
  fprintf(stream,"[WF.POA] Distance=%d\n",distance);
  fprintf(stream,"--------------------------------------------------------------------------------\n");
  const int segments_total = text_dag->segments_total;
  int segment_idx;
  for (segment_idx=0;segment_idx<segments_total;++segment_idx) {
    edit_wavefront_segment_t* const wavefront_segment = wavefront_poa->wavefront_segments[segment_idx];
    if (wavefront_segment != NULL) {
      edit_wavefront_poa_print_wavefront_segment(stream,wavefront_segment);
    }
  }
}











