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
 * DESCRIPTION: Dynamic-programming algorithm to compute Levenshtein alignment (edit)
 */

#include "edit_dp.h"

/*
 * Edit distance computation using dynamic programming
 */
void edit_dp_backtrace(
    score_matrix_t* const score_matrix,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    cigar_t* const cigar) {
  // Backtrace score-matrix
  int h = text_length;
  int v = pattern_length;
  edit_backtrace_score_matrix(score_matrix,pattern,&v,text,&h,cigar);
  // Add (possible) leading insertion/deletion
  cigar_add_leading_insertion(cigar,h);
  cigar_add_leading_deletion(cigar,v);
}
void edit_dp_compute(
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    cigar_t* const cigar,
    mm_allocator_t* const mm_allocator) {
  // Allocate
  score_matrix_t* const score_matrix =
      score_matrix_new(pattern_length,text_length,mm_allocator);
  int** const matrix = score_matrix->columns;
  // Init DP
  int h, v;
  for (v=0;v<=pattern_length;++v) matrix[0][v] = v; // No ends-free
  for (h=0;h<=text_length;++h) matrix[h][0] = h; // No ends-free
  // Compute DP
  for (h=1;h<=text_length;++h) {
    for (v=1;v<=pattern_length;++v) {
      int min = matrix[h-1][v-1] + (text[h-1]!=pattern[v-1]);
      min = MIN(min,matrix[h-1][v]+1); // Ins
      min = MIN(min,matrix[h][v-1]+1); // Del
      matrix[h][v] = min;
    }
  }
  // Compute backtrace
  edit_dp_backtrace(score_matrix,pattern,pattern_length,text,text_length,cigar);
  // Free
  // score_matrix_print(stderr,score_matrix,pattern,text); // DEBUG
  score_matrix_delete(score_matrix);
}
/*
 * Edit distance computation using dynamic programming (banded)
 */
void edit_dp_compute_banded(
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    const int bandwidth,
    cigar_t* const cigar,
    mm_allocator_t* const mm_allocator) {
  // Allocate
  score_matrix_t* const score_matrix =
      score_matrix_new(pattern_length,text_length,mm_allocator);
  // Parameters
  const int k_end = ABS(text_length-pattern_length)+1;
  const int effective_bandwidth = MAX(k_end,bandwidth);
  int** matrix = score_matrix->columns;
  int h, v;
  // Initialize
  matrix[0][0] = 0;
  for (v=1;v<=effective_bandwidth;++v) matrix[0][v] = v;
  // Compute DP
  for (h=1;h<=text_length;++h) {
    // Compute lo limit
    const bool lo_band = (h <= effective_bandwidth);
    const int lo = (lo_band) ? 1 : h - effective_bandwidth;
    matrix[h][lo-1] = (lo_band) ? h : INT16_MAX;
    // Compute hi limit
    const int hi = MIN(pattern_length,effective_bandwidth+h-1);
    if (h > 1) matrix[h-1][hi] = INT16_MAX;
    // Compute column
    for (v=lo;v<=hi;++v) {
      const int sub = matrix[h-1][v-1] + (text[h-1]!=pattern[v-1]); // Sub
      const int ins = matrix[h-1][v]; // Ins
      const int del = matrix[h][v-1]; // Del
      matrix[h][v] = MIN(MIN(ins,del)+1,sub);
    }
  }
  // Compute backtrace
  edit_dp_backtrace(score_matrix,pattern,pattern_length,text,text_length,cigar);
  // Free
  score_matrix_delete(score_matrix);
}
/*
 * Edit Backtrace
 */
void edit_backtrace_score_matrix(
    score_matrix_t* const score_matrix,
    const char* const pattern,
    int* const pattern_position,
    const char* const text,
    int* const text_position,
    cigar_t* const cigar) {
  // Parameters
  int** const matrix = score_matrix->columns;
  char* const operations = cigar->operations;
  int operation_idx = cigar->begin_offset;
  int h, v;
  // Compute backtrace
  h = *text_position;
  v = *pattern_position;
  while (h > 0 && v > 0) {
    if (matrix[h][v] == matrix[h][v-1]+1) {
      operations[--operation_idx] = 'D';
      --v;
    } else if (matrix[h][v] == matrix[h-1][v]+1) {
      operations[--operation_idx] = 'I';
      --h;
    } else if (matrix[h][v] == matrix[h-1][v-1]) {
      operations[--operation_idx] = 'M';
      --h;
      --v;
    } else if (matrix[h][v] == matrix[h-1][v-1]+1) {
      operations[--operation_idx] = 'X';
      --h;
      --v;
    } else {
      fprintf(stderr,"Edit score-matrix backtrace error: No backtrace operation found");
      exit(1);
    }
  }
  cigar->begin_offset = operation_idx;
  // Set up-most/left-most end of the backtrace
  *text_position = h;
  *pattern_position = v;
}
