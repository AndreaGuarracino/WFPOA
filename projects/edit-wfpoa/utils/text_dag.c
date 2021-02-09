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

#include "text_dag.h"

/*
 * Config
 */
#define DAG_SEGMENT_MAX_NODES 100
#define DAG_MAX_SEGMENTS 100

/*
 * Setup Segments
 */
text_dag_segment_t* text_dag_segment_new() {
  // Allocate
  text_dag_segment_t* const segment = malloc(sizeof(text_dag_segment_t));
  segment->prev = malloc(DAG_SEGMENT_MAX_NODES*sizeof(int));
  segment->prev_total = 0;
  segment->next = malloc(DAG_SEGMENT_MAX_NODES*sizeof(int));
  segment->next_total = 0;
  segment->sequence = NULL;
  segment->sequence_length = 0;
  // Return
  return segment;
}
void text_dag_segment_delete(
    text_dag_segment_t* const segment) {
  free(segment->sequence-1);
  free(segment->prev);
  free(segment->next);
  free(segment);
}
/*
 * Setup Text-DAG
 */
text_dag_t* text_dag_new() {
  // Allocate
  text_dag_t* const text_dag = malloc(sizeof(text_dag_t));
  text_dag->segments_ts = malloc(DAG_MAX_SEGMENTS*sizeof(text_dag_segment_t*));
  text_dag->segments_total = 0;
  // Return
  return text_dag;
}
void text_dag_delete(
    text_dag_t* const text_dag) {
  // Free individual segments
  int i;
  for (i=0;i<text_dag->segments_total;++i) {
    text_dag_segment_delete(text_dag->segments_ts[i]);
  }
  // Free DAG
  free(text_dag->segments_ts);
  free(text_dag);
}
/*
 * Accessors
 */
void text_dag_add_sequence(
    text_dag_t* const text_dag,
    char* const sequence,
    const char sentinel) {
  // Create new segment
  text_dag_segment_t* const segment = text_dag_segment_new();
  // Allocate and copy padded sequence
  const int sequence_lengh = strlen(sequence);
  char* const sequence_buffer = malloc(sequence_lengh+3);
  sequence_buffer[0] = sentinel;
  strncpy(sequence_buffer+1,sequence,sequence_lengh);
  sequence_buffer[sequence_lengh+1] = sentinel;
  sequence_buffer[sequence_lengh+2] = '\0';
  segment->sequence = sequence_buffer + 1;
  segment->sequence_length = sequence_lengh;
  // Insert new segment
  text_dag->segments_ts[text_dag->segments_total++] = segment;
}
void text_dag_add_connection(
    text_dag_t* const text_dag,
    const int node_a,
    const int node_b) {
  // Parameters
  text_dag_segment_t* const segment_a = text_dag->segments_ts[node_a];
  text_dag_segment_t* const segment_b = text_dag->segments_ts[node_b];
  // Connect segments
  segment_a->next[segment_a->next_total++] = node_b;
  segment_b->prev[segment_b->prev_total++] = node_a;
}
/*
 * Examples
 */
text_dag_t* text_dag_example1() {
  /*
   * 0 -> {"XACTX", NULL, {1,2}},
   * 1 -> {"XACCTGX", {0}, {3}},
   * 2 -> {"XGTX", {0}, {3}},
   * 3 -> {"XACTX", {1,2}, NULL}
   */
  // Allocate
  text_dag_t* const text_dag = text_dag_new();
  // Add segments (topologically sorted)
  text_dag_add_sequence(text_dag,"ACT",'X');
  text_dag_add_sequence(text_dag,"ACCTG",'X');
  text_dag_add_sequence(text_dag,"GT",'X');
  text_dag_add_sequence(text_dag,"ACT",'X');
  // Add connections (topologically sorted)
  text_dag_add_connection(text_dag,0,1);
  text_dag_add_connection(text_dag,0,2);
  text_dag_add_connection(text_dag,1,3);
  text_dag_add_connection(text_dag,2,3);
  // Return
  return text_dag;
}





