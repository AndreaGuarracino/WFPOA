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
  segment->prev_weight = malloc(DAG_SEGMENT_MAX_NODES*sizeof(int));
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
  free(segment->prev_weight);
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
  text_dag->consensus = malloc(DAG_MAX_SEGMENTS * sizeof(int));
  text_dag->consensus_len = 0;
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
    const int segment_id_a,
    const int segment_id_b,
    const int weight) {
  // Parameters
  text_dag_segment_t* const segment_a = text_dag->segments_ts[segment_id_a];
  text_dag_segment_t* const segment_b = text_dag->segments_ts[segment_id_b];

  // Connect segments
  // Check if the connection already exists
  for (int i = 0; i < segment_b->prev_total; ++i) {
      if (segment_b->prev[i] == segment_id_a) {
          // Increment weight
          segment_b->prev_weight[i] += weight;
          return;
      }
  }

  // New connection
  segment_a->next[segment_a->next_total++] = segment_id_b;
  segment_b->prev[segment_b->prev_total] = segment_id_a;
  segment_b->prev_weight[segment_b->prev_total++] = weight;
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

    int segment_rank = 0;

    while (stack_next_index != 0) {
        int segment_id = segment_ids_to_visit[--stack_next_index]; //top();

        text_dag->rank_to_segment_id[segment_rank++] = segment_id;

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
//        printf("segment_rank %d to segment id %d (%s)\n", i, segment_id, text_dag->segments_ts[segment_id]->sequence - 1);
//    }
}

int text_dag_branch_completion(
        text_dag_t* const text_dag,
        int64_t *scores,
        int64_t *predecessors,
        int segment_rank) {
    uint32_t i, j;

    int segment_id = text_dag->rank_to_segment_id[segment_rank];
    text_dag_segment_t* segment = text_dag->segments_ts[segment_id];
    //po_edge *tmp_edge;

    for (i = 0; i < segment->next_total; ++i) {
        int segment_id_next = segment->next[i];
        //tmp_edge = tmp_node->out_edges[i];

        for (j = 0; j < text_dag->segments_ts[segment_id_next]->prev_total; ++j) {
            int segment_id_prev = text_dag->segments_ts[segment_id_next]->prev[j];
            if (segment_id_prev != segment_id) {
                scores[segment_id_prev] = -1;
            }
        }
    }

    int64_t max_score = 0;
    int segment_id_with_max_score = 0;
    for (i = segment_rank + 1; i < text_dag->segments_total; ++i) {
        segment_id = text_dag->rank_to_segment_id[i];
        segment = text_dag->segments_ts[segment_id];

        scores[segment_id] = -1;
        predecessors[segment_id] = -1;

        for (j = 0; j < segment->prev_total; j++) {
            if (scores[segment->prev[j]] == -1) {
                continue;
            }

            int segment_id_prev = segment->prev[j];
            //tmp_edge = tmp_node->in_edges[j];

            if (scores[segment_id] < segment->prev_weight[j] ||
                (scores[segment_id] == segment->prev_weight[j] &&
                scores[predecessors[segment_id]] <= scores[segment_id_prev]
            )) {
                scores[segment_id] = segment->prev_weight[j];
                predecessors[segment_id] = segment_id_prev;
            }
        }

        if (predecessors[segment_id] != -1) {
            scores[segment_id] += scores[predecessors[segment_id]];
        }

        if (max_score < scores[segment_id]) {
            max_score = scores[segment_id];
            segment_id_with_max_score = segment_id;
        }
    }

    return segment_id_with_max_score;
}

void reverse(int* arr, int n) {
    for (int low = 0, high = n - 1; low < high; low++, high--) {
        int temp = arr[low];
        arr[low] = arr[high];
        arr[high] = temp;
    }
}

void text_dag_traverse_heaviest_bundle(
        text_dag_t* const text_dag) {
    int i, j;
    int segment_id;
    text_dag_segment_t* segment;

    int64_t *predecessors = malloc(text_dag->segments_total * sizeof(int64_t));
    int64_t *scores = malloc(text_dag->segments_total * sizeof(int64_t));
    for (i = 0; i < text_dag->segments_total; i++) {
        predecessors[i] = -1;
        scores[i] = -1;
    }

    int segment_id_with_max_score = 0;
    for (i = 0; i < text_dag->segments_total; ++i) {
        segment_id = text_dag->rank_to_segment_id[i];
        segment = text_dag->segments_ts[segment_id];

        for (j = 0; j < segment->prev_total; ++j) {
            int segment_id_prev = segment->prev[j];
            //tmp_edge = tmp_node->in_edges[j];

            if (scores[segment_id] < segment->prev_weight[j] ||
                (scores[segment_id] == segment->prev_weight[j] &&
                scores[predecessors[segment_id]] <= scores[segment_id_prev]
            )) {

                scores[segment_id] = segment->prev_weight[j];
                predecessors[segment_id] = segment_id_prev;
            }
        }

        if (predecessors[segment_id] != -1) {
            scores[segment_id] += scores[predecessors[segment_id]];
        }

        if (scores[segment_id_with_max_score] < scores[segment_id]) {
            segment_id_with_max_score = segment_id;
        }
    }

    if (text_dag->segments_ts[segment_id_with_max_score]->next_total > 0) {
        int* segment_id_to_rank = malloc(text_dag->segments_total * sizeof(int));

        for (i = 0; i < text_dag->segments_total; ++i) {
            segment_id_to_rank[text_dag->rank_to_segment_id[i]] = i;
        }

        do {
            segment_id_with_max_score = text_dag_branch_completion(text_dag,
                                                                   scores, predecessors,
                                                                   segment_id_to_rank[segment_id_with_max_score]);
        } while (text_dag->segments_ts[segment_id_with_max_score]->next_total != 0);
    }

    // Traceback
    text_dag->consensus_len = 0;

    while (predecessors[segment_id_with_max_score] != -1) {
        text_dag->consensus[text_dag->consensus_len++] = segment_id_with_max_score;
        segment_id_with_max_score = predecessors[segment_id_with_max_score];
    }
    text_dag->consensus[text_dag->consensus_len++] = segment_id_with_max_score;

    reverse(text_dag->consensus, text_dag->consensus_len);
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
  text_dag_add_connection(text_dag,0,1, 1+1);
  text_dag_add_connection(text_dag,0,2, 1+1);
  text_dag_add_connection(text_dag,1,3, 1+1);
  text_dag_add_connection(text_dag,2,3, 1+1);
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
  text_dag_add_connection(text_dag, 0, 1, 1+1);
  text_dag_add_connection(text_dag, 0, 2, 1+1);
  text_dag_add_connection(text_dag, 1, 3, 1+1);
  text_dag_add_connection(text_dag, 2, 3, 1+1);
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
  text_dag_add_connection(text_dag, 0, 1, 1+1);
  // Return
  return text_dag;
}
