/*
 *                             The MIT License
 *
 * Wavefront Alignments Algorithms
 * Copyright (c) 2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 * This file is part of Wavefront Alignments Algorithms.
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
 * PROJECT: Wavefront Alignments Algorithms
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Edit-Distance based wavefront alignment algorithm
 */

#ifndef EDIT_WAVEFRONT_H_
#define EDIT_WAVEFRONT_H_

#include "utils/commons.h"
#include "utils/text_dag.h"
#include "system/mm_allocator.h"

/*
 * Translate k and offset to coordinates h,v
 */
#define EWAVEFRONT_V(k,offset) ((offset)-(k))
#define EWAVEFRONT_H(k,offset) (offset)

#define EWAVEFRONT_DIAGONAL(h,v) ((h)-(v))
#define EWAVEFRONT_OFFSET(h,v)   (h)

#define MAX(a,b) (((a)>=(b))?(a):(b))
#define ABS(a) (((a)>=0)?(a):-(a))

#define EWAVEFRONT_OFFSET_NULL -10

/*
 * Individual Edit Wavefront
 */
typedef int16_t ewf_offset_t;  // Edit Wavefront Offset
typedef struct {
  int segment_idx;
  int distance;
  int k;
  ewf_offset_t offset;
} edit_wavefront_locator_t;
typedef struct {
  edit_wavefront_locator_t previous_wf_end;
  edit_wavefront_locator_t current_wf_begin;
  bool disabled;
} edit_wavefront_control_t;
typedef struct {
  // Offsets memory
  int lo_max;                  // Max allocated lowest diagonal (inclusive)
  int hi_max;                  // Max allocated highest diagonal (inclusive)
  ewf_offset_t* offsets_mem;   // Offsets memory
  // Offsets
  int lo;                      // Effective lowest diagonal (inclusive)
  int hi;                      // Effective highest diagonal (inclusive)
  ewf_offset_t* offsets;       // Offsets
} edit_wavefront_t;

/*
 * Edit Wavefront-Segments
 */
typedef struct {
  // Index
  int index;
  // Sequences
  char* pattern;
  int pattern_length;
  text_dag_segment_t* text_segment;
  // Wavefront
  edit_wavefront_t** wavefronts;
  int wf_distance_min;
  int wf_distance_max;
  // Control
  edit_wavefront_control_t* control_mem;
  edit_wavefront_control_t* control;
  int num_valid_offsets;
  // MM
  mm_allocator_t* mm_allocator;
} edit_wavefront_segment_t;

/*
 * Edit Wavefront-POA
 */
typedef struct {
  // Segment wavefronts
  edit_wavefront_segment_t** wavefront_segments;
  // MM
  mm_allocator_t* mm_allocator;
} edit_wavefront_poa_t;

/*
 * Individual Edit Wavefront
 */
edit_wavefront_t* edit_wavefront_new(
    const int lo_max,
    const int hi_max,
    const int lo,
    const int hi,
    mm_allocator_t* const mm_allocator);
void edit_wavefront_delete(
    edit_wavefront_t* const wavefront,
    mm_allocator_t* const mm_allocator);

/*
 * Edit Wavefront-Segments
 */
edit_wavefront_segment_t* edit_wavefront_segment_new(
    char* const pattern,
    const int pattern_length,
    text_dag_segment_t* const text_segment,
    mm_allocator_t* const mm_allocator);
void edit_wavefront_segment_delete(
    edit_wavefront_segment_t* const wavefronts_segment);

bool edit_wavefront_segment_is_active(
    edit_wavefront_segment_t* const wavefronts_segment,
    const int distance);

/*
 * Wavefront-POA Setup
 */
edit_wavefront_poa_t* edit_wavefront_poa_new(
    mm_allocator_t* const mm_allocator);
void edit_wavefront_poa_delete(
    edit_wavefront_poa_t* const wavefront_poa);

#endif /* EDIT_WAVEFRONT_H_ */
