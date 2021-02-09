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
 * DESCRIPTION: Dynamic-programming algorithm to compute partial order
 *              alignment (POA) using the Levenshtein distance (edit)
 */

#include "edit_dp_poa.h"
#include "edit_dp.h"
#include "alignment/score_matrix.h"

/*
 * POA Backtrace (edit distance using dynamic programming)
 */
void edit_dp_poa_backtrace(
    score_matrix_t** const score_matrices,
    const char* const pattern,
    const int pattern_length,
    text_dag_t* const text_dag,
    cigar_t* const cigar) {
  // Clear CIGAR
  cigar_clear(cigar);
  // Backtrace from last segment in the text-DAG
  const int segments_total = text_dag->segments_total;
  int segment_idx = segments_total - 1; // Last segment
  int v = pattern_length;
  while (segment_idx >= 0) {
    // Fetch segment
    text_dag_segment_t* const segment = text_dag->segments_ts[segment_idx];
    int h = segment->sequence_length;
    const char* const text = segment->sequence;
    score_matrix_t* const score_matrix = score_matrices[segment_idx];
    // Backtrace segment-region
    edit_backtrace_score_matrix(score_matrix,pattern,&v,text,&h,cigar);
    cigar_add_leading_insertion(cigar,h); // Add (possible) leading insertion
    cigar_add_segment(cigar,segment_idx); // Add the segment-index into the CIGAR
    // Compute previous segment (i.e., which segment we came from)
    int** const matrix = score_matrix->columns;
    segment_idx = -1; // No previous segment
    int i;
    for (i=0;i<segment->prev_total;++i) {
      const int prev_idx = segment->prev[i];
      text_dag_segment_t* const prev_segment = text_dag->segments_ts[prev_idx];
      const int prev_length = prev_segment->sequence_length;
      int** const prev_matrix = score_matrices[prev_idx]->columns;
      if (prev_matrix[prev_length][v] == matrix[0][v]) {
        segment_idx = prev_idx;
        break;
      }
    }
  }
  // No previous segment found, finish backtrace
  cigar_add_leading_deletion(cigar,v);
}
/*
 * POA Edit distance computation using dynamic programming
 */
void edit_dp_poa_compute(
    const char* const pattern,
    const int pattern_length,
    text_dag_t* const text_dag,
    cigar_t* const cigar,
    mm_allocator_t* const mm_allocator) {
  // Parameters
  const int segments_total = text_dag->segments_total;
  // Allocate score-matrices
  score_matrix_t** const score_matrices =
      mm_allocator_calloc(mm_allocator,segments_total,score_matrix_t*,false);
  int i;
  for (i=0;i<segments_total;++i) {
    score_matrices[i] = score_matrix_new(pattern_length,
        text_dag->segments_ts[i]->sequence_length,mm_allocator);
  }
  // Compute score-matrix segment-wise
  int segment_idx;
  for (segment_idx=0;segment_idx<segments_total;++segment_idx) {
    // Parameters
    text_dag_segment_t* const segment = text_dag->segments_ts[segment_idx];
    const int text_length = segment->sequence_length;
    const char* const text = segment->sequence;
    int** const matrix = score_matrices[segment_idx]->columns;
    int h, v;
    // Init segment-region (first column and row)
    if (segment_idx == 0) {
      for (v=0;v<=pattern_length;++v) matrix[0][v] = v;
      for (h=0;h<=text_length;++h) matrix[h][0] = h;
    } else {
      for (v=0;v<=pattern_length;++v) matrix[0][v] = SCORE_MAX;
      int i;
      for (i=0;i<segment->prev_total;++i) {
        const int prev_idx = segment->prev[i];
        text_dag_segment_t* const prev_segment = text_dag->segments_ts[prev_idx];
        const int prev_length = prev_segment->sequence_length;
        int** const prev_matrix = score_matrices[prev_idx]->columns;
        for (v=0;v<=pattern_length;++v) {
          matrix[0][v] = MIN(matrix[0][v],prev_matrix[prev_length][v]);
        }
      }
      for (h=0;h<=text_length;++h) matrix[h][0] = matrix[0][0] + h;
    }
    // Compute score-matrix for current segment-region
    for (h=1;h<=text_length;++h) {
      for (v=1;v<=pattern_length;++v) {
        int min = matrix[h-1][v-1] + (text[h-1]!=pattern[v-1]);
        min = MIN(min,matrix[h-1][v]+1); // Ins
        min = MIN(min,matrix[h][v-1]+1); // Del
        matrix[h][v] = min;
      }
    }
  }
  // Compute backtrace
  edit_dp_poa_backtrace(score_matrices,pattern,pattern_length,text_dag,cigar);
  // DEBUG
  score_matrices_print(stderr,score_matrices,pattern,pattern_length,text_dag);
  // Free
  for (i=0;i<segments_total;++i) {
    score_matrix_delete(score_matrices[i]);
  }
  mm_allocator_free(mm_allocator,score_matrices);
}





