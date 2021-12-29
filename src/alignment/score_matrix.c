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
 * DESCRIPTION: Score matrix for alignment using dynamic programming
 */

#include "score_matrix.h"

/*
 * Setup
 */
score_matrix_t* score_matrix_new(
    const int pattern_length,
    const int text_length,
    mm_allocator_t* const mm_allocator) {
  // Allocate handler
  score_matrix_t* const score_matrix = mm_allocator_alloc(mm_allocator,score_matrix_t);
  // Allocate DP matrix
  int h;
  score_matrix->num_rows = pattern_length + 1;
  const int num_columns = text_length + 1;
  score_matrix->num_columns = num_columns;
  score_matrix->columns = mm_allocator_malloc(mm_allocator,(text_length+1)*sizeof(int*)); // Columns
  for (h=0;h<num_columns;++h) {
    score_matrix->columns[h] = mm_allocator_calloc(mm_allocator,pattern_length+1,int,false); // Rows
  }
  // MM
  score_matrix->mm_allocator = mm_allocator;
  // Return
  return score_matrix;
}
void score_matrix_delete(
    score_matrix_t* const score_matrix) {
  // Parameters
  mm_allocator_t* const mm_allocator = score_matrix->mm_allocator;
  // DP matrix
  const int num_columns = score_matrix->num_columns;
  int h;
  for (h=0;h<num_columns;++h) {
    mm_allocator_free(mm_allocator,score_matrix->columns[h]);
  }
  mm_allocator_free(mm_allocator,score_matrix->columns);
  // Handler
  mm_allocator_free(mm_allocator,score_matrix);
}
/*
 * Display
 */
void score_matrix_print_score(
    FILE* const stream,
    const int score) {
  if (-1 < score && score < 10000) {
    fprintf(stream," %3d ",score);
  } else {
    fprintf(stream,"  *  ");
  }
}
void score_matrix_print_char(
    FILE* const stream,
    const char c) {
  fprintf(stream,"  %c  ",c);
}
void score_matrix_print(
    FILE* const stream,
    const score_matrix_t* const score_matrix,
    const char* const pattern,
    const char* const text) {
  // Parameters
  int** const matrix = score_matrix->columns;
  const int num_columns = score_matrix->num_columns;
  const int num_rows = score_matrix->num_rows;
  int h;
  // Print Header
  fprintf(stream,"       ");
  for (h=0;h<num_columns-1;++h) {
    score_matrix_print_char(stream,text[h]);
  }
  fprintf(stream,"\n ");
  for (h=0;h<num_columns;++h) {
    score_matrix_print_score(stream,h);
  }
  fprintf(stream,"\n ");
  for (h=0;h<num_columns;++h) {
    score_matrix_print_score(stream,matrix[h][0]);
  }
  fprintf(stream,"\n");
  // Print Rows
  int v;
  for (v=1;v<num_rows;++v) {
    fprintf(stream,"%c",pattern[v-1]);
    for (h=0;h<num_columns;++h) {
      score_matrix_print_score(stream,matrix[h][v]);
    }
    fprintf(stream,"\n");
  }
  fprintf(stream,"\n");
}
void score_matrices_print(
    FILE* const stream,
    score_matrix_t** const score_matrices,
    const char* const pattern,
    const int pattern_length,
    text_dag_t* const text_dag) {
  // Parameters
  int segment_idx, v, h;
  const int segments_total = text_dag->segments_total;
  // Print linearized DAG
  fprintf(stream,"       ");
  for (segment_idx=0;segment_idx<segments_total;++segment_idx) {
    text_dag_segment_t* const segment = text_dag->segments_ts[segment_idx];
    const int text_length = segment->sequence_length;
    const char* const text = segment->sequence;
    for (h=0;h<text_length;++h) {
      score_matrix_print_char(stream,text[h]);
    }
    if (segment_idx != segments_total-1) score_matrix_print_score(stream,-1); // Separator
  }
  fprintf(stream,"\n");
  // Print segment-regions row-wise
  for (v=0;v<=pattern_length;++v) {
    fprintf(stream,"%c",(v > 0) ? pattern[v-1] : ' ');
    for (segment_idx=0;segment_idx<segments_total;++segment_idx) {
      text_dag_segment_t* const segment = text_dag->segments_ts[segment_idx];
      const int text_length = segment->sequence_length;
      int** const matrix = score_matrices[segment_idx]->columns;
      for (h=0;h<=text_length;++h) {
        score_matrix_print_score(stream,matrix[h][v]);
      }
    }
    fprintf(stream,"\n");
  }
  fprintf(stream,"\n");
}






























