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
 * DESCRIPTION: Simple hypertext data-structure (DAG) topologically sorted
 */

#ifndef TEXT_DAG_H_
#define TEXT_DAG_H_

#include "commons.h"

/*
 * Text DAG (Topologically sorted)
 */
typedef struct {
  // Sequence
  char* sequence;
  int sequence_length;
  // Links
  int* prev;
  int prev_total;
  int* next;
  int next_total;
} text_dag_segment_t;
typedef struct {
  text_dag_segment_t** segments_ts; // Topologically Sorted (todo use rank_to_segment_id)
  int* rank_to_segment_id;          // From ranks (topological sorted) to segment ids
  int segments_total;               // Total number of segments
} text_dag_t;

/*
 * Setup
 */
text_dag_t* text_dag_new();
void text_dag_delete(
    text_dag_t* const text_dag);

/*
 * Accessors
 */
void text_dag_add_segment(
    text_dag_t* const text_dag,
    char* const sequence,
    const char sentinel);
void text_dag_add_connection(
    text_dag_t* const text_dag,
    const int node_a,
    const int node_b);
void text_dag_topological_sort(
        text_dag_t* const text_dag);
/*
 * Examples
 */
text_dag_t* text_dag_example1();
text_dag_t* text_dag_example2();
text_dag_t* text_dag_example3();

#endif /* TEXT_DAG_H_ */
