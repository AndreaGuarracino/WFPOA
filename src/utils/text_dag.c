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
  text_dag->rank_to_segment_id = malloc(DAG_MAX_SEGMENTS*sizeof(int));
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
  free(text_dag->rank_to_segment_id);
  free(text_dag);
}
/*
 * Accessors
 */
void text_dag_add_segment(
    text_dag_t* const text_dag,
    char* const sequence,
    const char sentinel) {
  // Create new segment
  text_dag_segment_t* const segment = text_dag_segment_new();
  // Allocate and copy padded sequence
  const int sequence_length = strlen(sequence);
  char* const sequence_buffer = malloc(sequence_length+3);
  sequence_buffer[0] = sentinel;
  strncpy(sequence_buffer+1,sequence,sequence_length);
  sequence_buffer[sequence_length+1] = sentinel;
  sequence_buffer[sequence_length+2] = '\0';
  segment->sequence = sequence_buffer + 1;
  segment->sequence_length = sequence_length;
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

void text_dag_topological_sort(
        text_dag_t* const text_dag){
    // Clear ranks
    for(int i = 0; i < text_dag->segments_total; ++i) {
        text_dag->rank_to_segment_id[i] = 0;
    }

    int* segment_ids_to_visit = malloc(text_dag->segments_total * sizeof(int)); //todo: implement a proper stack
    int stack_next_index = 0;

    // O(V)
    int* in_degree = malloc(text_dag->segments_total*sizeof(int));
    for(int i = 0; i < text_dag->segments_total; ++i) {
        in_degree[i] = text_dag->segments_ts[i]->prev_total;

        // Kahn’s algorithm: it needs the list of "start nodes" which have no incoming edges
        if (in_degree[i] == 0) {
            segment_ids_to_visit[stack_next_index++] = i;
        }
    }

    // Initialize count of visited vertices
    int num_visited_vertices = 0;

    int rank = 0;

    while (stack_next_index != 0) {
        int segment_id = segment_ids_to_visit[--stack_next_index]; //top();

        text_dag->rank_to_segment_id[rank++] = segment_id;

        for (int j = 0; j < text_dag->segments_ts[segment_id]->next_total; ++j) {
            int segment_id_next = text_dag->segments_ts[segment_id]->next[j];

            --in_degree[segment_id_next];

            if (in_degree[segment_id_next] == 0) {
                segment_ids_to_visit[stack_next_index++] = segment_id_next;
            }
        }

        num_visited_vertices += 1;
    }

    // Check if there was a cycle
    assert(
        (num_visited_vertices == text_dag->segments_total) &&
        "[wfpoa::text_dag_topological_sort] error: graph is not a DAG");

//    for (int i = 0; i < text_dag->segments_total; ++i) {
//        int segment_id = text_dag->rank_to_segment_id[i];
//        printf("rank %d to segment id %d (%s)\n", i, segment_id, text_dag->segments_ts[segment_id]->sequence - 1);
//    }
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
  text_dag_add_segment(text_dag,"ACT",'X');
  text_dag_add_segment(text_dag,"ACCTG",'X');
  text_dag_add_segment(text_dag,"GT",'X');
  text_dag_add_segment(text_dag,"ACT",'X');
  // Add connections (topologically sorted)
  text_dag_add_connection(text_dag,0,1);
  text_dag_add_connection(text_dag,0,2);
  text_dag_add_connection(text_dag,1,3);
  text_dag_add_connection(text_dag,2,3);
  // Return
  return text_dag;
}

text_dag_t *text_dag_example2() {
  /*
   * 0 -> {"XAAAX", NULL, {1,2}},
   * 1 -> {"XTTTX", {0}, {3}},
   * 2 -> {"XGGGX", {0}, {3}},
   * 3 -> {"XAAAX", {1,2}, NULL}
   */
  // Allocate
  text_dag_t *const text_dag = text_dag_new();
  // Add segments (topologically sorted)
  text_dag_add_segment(text_dag, "AAA", 'X');
  text_dag_add_segment(text_dag, "TTT", 'X');
  text_dag_add_segment(text_dag, "GGG", 'X');
  text_dag_add_segment(text_dag, "AAA", 'X');
  // Add connections (topologically sorted)
  text_dag_add_connection(text_dag, 0, 1);
  text_dag_add_connection(text_dag, 0, 2);
  text_dag_add_connection(text_dag, 1, 3);
  text_dag_add_connection(text_dag, 2, 3);
  // Return
  return text_dag;
}

text_dag_t *text_dag_example3() {
  /*
   * 0 -> {"XAX", NULL, {1}},
   * 3 -> {"XGX", {0}, NULL}
   */
  // Allocate
  text_dag_t *const text_dag = text_dag_new();
  // Add segments (topologically sorted)
  text_dag_add_segment(text_dag, "A", 'X');
  text_dag_add_segment(text_dag, "G", 'X');
  // Add connections (topologically sorted)
  text_dag_add_connection(text_dag, 0, 1);
  // Return
  return text_dag;
}
