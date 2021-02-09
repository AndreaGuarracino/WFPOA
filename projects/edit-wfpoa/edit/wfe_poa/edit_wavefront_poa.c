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

/*
 * Constants
 */
#define EDIT_WF_POA_MAX_SEGMENTS 1000
#define EDIT_WF_POA_MAX_SEGMENT_WAVEFRONTS 1000

/*
 * Individual Edit Wavefront
 */
edit_wavefront_t* edit_wavefront_new(
    const int lo_max,
    const int hi_max,
    const int lo,
    const int hi,
    mm_allocator_t* const mm_allocator) {
  // Allocate
  edit_wavefront_t* const wavefront = mm_allocator_alloc(mm_allocator,edit_wavefront_t);
  // Offsets
  const int wavefront_length = hi_max - lo_max + 2; // (+1) for k=0
  wavefront->offsets_mem = mm_allocator_calloc(
      mm_allocator,wavefront_length,ewf_offset_t,false);
  wavefront->offsets = wavefront->offsets_mem - lo_max; // Center at k=0
  wavefront->lo_max = lo_max;
  wavefront->hi_max = hi_max;
  wavefront->lo = lo;
  wavefront->hi = hi;
  // Return
  return wavefront;
}
void edit_wavefront_delete(
    edit_wavefront_t* const wavefront,
    mm_allocator_t* const mm_allocator) {
  // Free
  mm_allocator_free(mm_allocator,wavefront->offsets_mem);
  mm_allocator_free(mm_allocator,wavefront);
}
/*
 * Edit Wavefront-Segments
 */
edit_wavefront_segment_t* edit_wavefront_segment_new(
    char* const pattern,
    const int pattern_length,
    text_dag_segment_t* const text_segment,
    mm_allocator_t* const mm_allocator) {
  // Allocate
  edit_wavefront_segment_t* const wavefronts_segment =
      mm_allocator_alloc(mm_allocator,edit_wavefront_segment_t);
  // Sequences
  wavefronts_segment->pattern = pattern;
  wavefronts_segment->pattern_length = pattern_length;
  wavefronts_segment->text_segment = text_segment;
  // Wavefronts
  wavefronts_segment->wavefronts = mm_allocator_calloc(mm_allocator,
      EDIT_WF_POA_MAX_SEGMENT_WAVEFRONTS,edit_wavefront_t*,true);
  wavefronts_segment->wf_distance_min = -1;
  wavefronts_segment->wf_distance_max = -1;
  // Control
  wavefronts_segment->control_mem = mm_allocator_calloc(
      mm_allocator,pattern_length+text_segment->sequence_length+1,
      edit_wavefront_control_t,true);
  wavefronts_segment->control = wavefronts_segment->control_mem + pattern_length; // Center at k=0
  wavefronts_segment->num_valid_offsets = 0;
  // MM
  wavefronts_segment->mm_allocator = mm_allocator;
  // Return
  return wavefronts_segment;
}
void edit_wavefront_segment_delete(
    edit_wavefront_segment_t* const wavefronts_segment) {
  // Parameters
  mm_allocator_t* const mm_allocator = wavefronts_segment->mm_allocator;
  // Free
  int i;
  for (i=0;i<EDIT_WF_POA_MAX_SEGMENT_WAVEFRONTS;++i) {
    if (wavefronts_segment->wavefronts[i] != NULL) {
      edit_wavefront_delete(wavefronts_segment->wavefronts[i],mm_allocator);
    }
  }
  mm_allocator_free(mm_allocator,wavefronts_segment->wavefronts);
  mm_allocator_free(mm_allocator,wavefronts_segment->control_mem);
  mm_allocator_free(mm_allocator,wavefronts_segment);
}
bool edit_wavefront_segment_is_active(
    edit_wavefront_segment_t* const wavefronts_segment,
    const int distance) {
  // Check if wavefront-segment is open
  if (wavefronts_segment == NULL) return false;
  // Check if there is any active offset
  if (wavefronts_segment->num_valid_offsets == 0) return false;
  // Check if wavefront for distance is not NULL
  edit_wavefront_t* const wavefront = wavefronts_segment->wavefronts[distance];
  return (wavefront != NULL);
}
/*
 * Wavefront-POA Setup
 */
edit_wavefront_poa_t* edit_wavefront_poa_new(
    mm_allocator_t* const mm_allocator) {
  // Allocate
  edit_wavefront_poa_t* const wavefront_poa =
      mm_allocator_alloc(mm_allocator,edit_wavefront_poa_t);
  // Segment wavefronts
  wavefront_poa->wavefront_segments = mm_allocator_calloc(mm_allocator,
      EDIT_WF_POA_MAX_SEGMENTS,edit_wavefront_segment_t*,true);
  // MM
  wavefront_poa->mm_allocator = mm_allocator;
  // Return
  return wavefront_poa;
}
void edit_wavefront_poa_delete(
    edit_wavefront_poa_t* const wavefront_poa) {
  // Parameters
  mm_allocator_t* const mm_allocator = wavefront_poa->mm_allocator;
  // Free
  int i;
  for (i=0;i<EDIT_WF_POA_MAX_SEGMENTS;++i) {
    if (wavefront_poa->wavefront_segments[i] != NULL) {
      mm_allocator_free(mm_allocator,wavefront_poa->wavefront_segments[i]);
    }
  }
  mm_allocator_free(mm_allocator,wavefront_poa->wavefront_segments);
  mm_allocator_free(mm_allocator,wavefront_poa);
}


