#include "graph.h"
#include <stdlib.h>

#include <stdio.h> // for printf
#include <stdbool.h>
#include <assert.h>

#define CAUTIOUS_MODE

bool po_node_successor(
        po_node *const node,
        uint32_t *node_id,
        uint32_t label) {

    po_edge *tmp_edge;
    uint32_t j;

    for (uint32_t i = 0; i < node->num_out_edges; ++i) {
        tmp_edge = node->out_edges[i];

        for (j = 0; j < tmp_edge->num_sequence_labels; ++j) {
            if (tmp_edge->sequence_labels[j] == label) {
                *node_id = tmp_edge->end_node_id;
                return true;
            }
        }
    }

    return false;
}


void po_edge_add_sequence(
        po_edge *const edge,
        uint32_t label,
        uint32_t weight) {
    edge->sequence_labels[edge->num_sequence_labels++] = label;
    edge->total_weight += weight;
}

void po_graph_init(
        po_graph *const graph,
        const uint32_t num_initial_nodes
) {
    graph->num_sequences = 0;

    graph->nodes = malloc(num_initial_nodes * sizeof(po_node)); //todo: make it growable
    graph->num_nodes = 0;

    graph->rank_to_node_id = malloc(num_initial_nodes * sizeof(uint32_t)); //todo: make it growable
    graph->num_rank_to_node_id = 0;

    graph->sequences_begin_nodes_ids = malloc(3 * sizeof(uint32_t)); //todo: make it growable

    //uint32_t* consensus = malloc(num_initial_nodes * sizeof(uint32_t)); //todo: make it growable
    uint32_t consensus_len = 0;
}

void po_graph_create_and_init_node(
        po_node *const node,
        uint32_t id,
        const char character
) {
    node->id = id;
    node->character = character;

    node->in_edges = malloc(3 * sizeof(po_edge)); //todo: make it growable
    node->num_in_edges = 0;

    node->out_edges = malloc(3 * sizeof(po_edge)); //todo: make it growable
    node->num_out_edges = 0;

    node->aligned_nodes_ids = malloc(3 * sizeof(uint32_t)); //todo: make it growable
    node->num_aligned_nodes_ids = 0;
}

void po_graph_create_and_int_edge(
        po_edge *const edge,
        uint32_t begin_node_id, uint32_t end_node_id,
        uint32_t label,
        uint32_t weight
) {
    edge->begin_node_id = begin_node_id;
    edge->end_node_id = end_node_id;

    edge->sequence_labels = malloc(3 * sizeof(uint32_t)); //todo: make it growable
    edge->sequence_labels[0] = label;
    edge->num_sequence_labels = 1;

    edge->total_weight = weight;
}

uint32_t po_graph_add_node(
        po_graph *const graph,
        char character) {
    uint32_t node_id = graph->num_nodes++;
    po_graph_create_and_init_node(graph->nodes + node_id, node_id, character);
    return node_id;
}

void po_graph_add_edge(
        po_graph *const graph,
        uint32_t begin_node_id, uint32_t end_node_id,
        uint32_t weight) {

    po_node *begin_node = graph->nodes + begin_node_id;

    po_edge *tmp_edge;
    for (uint32_t i = 0; i < begin_node->num_out_edges; ++i) {
        tmp_edge = begin_node->out_edges[i];

        if (tmp_edge->end_node_id == end_node_id) {
            po_edge_add_sequence(tmp_edge, graph->num_sequences, weight);
            return;
        }
    }

    po_node *end_node = graph->nodes + end_node_id;

    po_edge *edge = malloc(sizeof(po_edge));
    po_graph_create_and_int_edge(edge, begin_node_id, end_node_id, graph->num_sequences, weight);
    begin_node->out_edges[begin_node->num_out_edges++] = edge;
    end_node->in_edges[end_node->num_in_edges++] = edge;
}

int32_t po_graph_add_sequence(
        po_graph *const graph,
        const char *sequence,
        const uint32_t *weights,
        uint32_t begin, uint32_t end) {

    if (begin == end) {
        return -1;
    }

    int32_t first_node_id = po_graph_add_node(graph, sequence[begin]);
    uint32_t node_id;
    for (uint32_t i = begin + 1; i < end; ++i) {
        node_id = po_graph_add_node(graph, sequence[i]);

        // both nodes contribute to edge weight
        po_graph_add_edge(graph, node_id - 1, node_id, weights[i - 1] + weights[i]);
    }

    return first_node_id;
}

void po_graph_add_alignment(
        po_graph *const graph,
        const alignment *alignment,
        const char *sequence, uint32_t sequence_size,
        const uint32_t *weights) {

#ifdef CAUTIOUS_MODE
    if (sequence_size == 0) {
        return;
    }

    /* todo or remove
     if (sequence_size != weights.size()) {
        throw std::invalid_argument("[spoa::Graph::add_alignment] error: "
                                    "sequence and weights are of unequal size!");
    }*/
#endif
    int32_t begin_node_id;
    if (alignment->num_pairs == 0) { //  no alignment
        begin_node_id = po_graph_add_sequence(graph, sequence, weights, 0, sequence_size);
    } else {
        uint32_t i, j;

        uint32_t *valid_seq_ids = malloc(sequence_size * sizeof(pair)); //todo: make it growable
        //uint32_t num_allocated_valid_seq_ids;
        uint32_t num_valid_seq_ids = 0;

        for (i = 0; i < alignment->num_pairs; ++i) {
            if (alignment->aligned_pairs[i].second != -1) {
                valid_seq_ids[num_valid_seq_ids++] = alignment->aligned_pairs[i].second;
            }
        }

#ifdef CAUTIOUS_MODE
        assert(valid_seq_ids[0] <= sequence_size);
        assert(valid_seq_ids[num_valid_seq_ids - 1] + 1 <= sequence_size);
#endif

        uint32_t tmp = graph->num_nodes;
        begin_node_id = po_graph_add_sequence(graph, sequence, weights, 0, valid_seq_ids[0]);
        int32_t head_node_id = (tmp == graph->num_nodes ? -1 : graph->num_nodes - 1);

        int32_t tail_node_id = po_graph_add_sequence(
                graph, sequence, weights,
                valid_seq_ids[num_valid_seq_ids - 1] + 1, sequence_size);

        int32_t new_node_id = -1;
        float prev_weight = head_node_id == -1 ? 0 : (float) weights[valid_seq_ids[0] - 1];

        uint32_t aligned_node_id;
        po_node *tmp_node;

        for (i = 0; i < alignment->num_pairs; ++i) {
            pair *pair = alignment->aligned_pairs + i;

            if (pair->second == -1) {
                continue;
            }

            char letter = sequence[pair->second];
            if (pair->first == -1) {
                new_node_id = po_graph_add_node(graph, letter);
            } else {
                tmp_node = graph->nodes + pair->first;

                if (tmp_node->character == letter) {
                    new_node_id = pair->first;
                } else {
                    int32_t aligned_to_node_id = -1;
                    for (j = 0; j < tmp_node->num_aligned_nodes_ids; ++j) {
                        aligned_node_id = tmp_node->aligned_nodes_ids[j];

                        if (graph->nodes[aligned_node_id].character == letter) {
                            aligned_to_node_id = aligned_node_id;
                            break;
                        }
                    }

                    if (aligned_to_node_id == -1) {
                        new_node_id = po_graph_add_node(graph, letter);

                        for (j = 0; j < tmp_node->num_aligned_nodes_ids; ++j) {
                            aligned_node_id = tmp_node->aligned_nodes_ids[j];

                            graph->nodes[new_node_id].aligned_nodes_ids[
                                    graph->nodes[new_node_id].num_aligned_nodes_ids++
                            ] = aligned_node_id;

                            graph->nodes[aligned_node_id].aligned_nodes_ids[
                                    graph->nodes[aligned_node_id].num_aligned_nodes_ids++
                            ] = new_node_id;
                        }

                        graph->nodes[new_node_id].aligned_nodes_ids[
                                graph->nodes[new_node_id].num_aligned_nodes_ids++
                        ] = pair->first;

                        tmp_node->aligned_nodes_ids[
                                tmp_node->num_aligned_nodes_ids++
                        ] = new_node_id;
                    } else {
                        new_node_id = aligned_to_node_id;
                    }
                }
            }

            if (begin_node_id == -1) {
                begin_node_id = new_node_id;
            }

            if (head_node_id != -1) {
                // both nodes contribute to edge weight
                po_graph_add_edge(graph,
                                  head_node_id, new_node_id,
                                  prev_weight + weights[pair->second]);
            }

            head_node_id = new_node_id;
            prev_weight = weights[pair->second];
        }

        if (tail_node_id != -1) {
            // both nodes contribute to edge weight
            po_graph_add_edge(graph,
                              head_node_id, tail_node_id,
                              prev_weight + weights[valid_seq_ids[num_valid_seq_ids - 1] + 1]);
        }
    }

    graph->sequences_begin_nodes_ids[graph->num_sequences] = begin_node_id;
    ++graph->num_sequences;

    topological_sort(graph);
}

void topological_sort(po_graph *const graph) {
    free(graph->rank_to_node_id);//todo: manage in a clever way; graph->rank_to_node_id.clear()
    graph->rank_to_node_id = malloc(graph->num_nodes * sizeof(uint32_t)); //todo: make it growable
    graph->num_rank_to_node_id = 0;

    uint32_t e, a;

    // 0 - unmarked, 1 - temporarily marked, 2 - permanently marked
    uint8_t *node_marks = calloc(graph->num_nodes, sizeof(uint8_t));
    bool *do_not_check_aligned_nodes = calloc(graph->num_nodes, sizeof(bool));

    uint32_t *nodes_to_visit = malloc(graph->num_nodes * sizeof(uint32_t)); //todo: implement a proper stack
    uint32_t stack_next_index = 0;

    uint32_t node_id, aligned_node_id;
    po_node *tmp_node;
    po_edge *tmp_edge;
    bool valid;

    for (uint32_t i = 0; i < graph->num_nodes; ++i) {
        if (node_marks[i] != 0) {
            continue;
        }

        nodes_to_visit[stack_next_index++] = i;
        while (stack_next_index != 0) {
            node_id = nodes_to_visit[stack_next_index - 1]; //top();
            tmp_node = graph->nodes + node_id;

            valid = true;

            if (node_marks[node_id] != 2) {
                for (e = 0; e < tmp_node->num_in_edges; ++e) {
                    tmp_edge = tmp_node->in_edges[e];
                    if (node_marks[tmp_edge->begin_node_id] != 2) {
                        nodes_to_visit[stack_next_index++] = tmp_edge->begin_node_id;
                        valid = false;
                    }
                }

                if (!do_not_check_aligned_nodes[node_id]) {
                    for (a = 0; a < tmp_node->num_aligned_nodes_ids; ++a) {
                        aligned_node_id = tmp_node->aligned_nodes_ids[a];

                        if (node_marks[aligned_node_id] != 2) {
                            nodes_to_visit[stack_next_index++] = aligned_node_id;
                            do_not_check_aligned_nodes[aligned_node_id] = true;
                            valid = false;
                        }
                    }
                }

#ifdef CAUTIOUS_MODE
                assert((valid || node_marks[node_id] != 1) && "Graph is not a DAG!");
#endif

                if (valid) {
                    node_marks[node_id] = 2;
                    if (!do_not_check_aligned_nodes[node_id]) {
                        graph->rank_to_node_id[graph->num_rank_to_node_id++] = node_id;
                        for (a = 0; a < tmp_node->num_aligned_nodes_ids; ++a) {
                            graph->rank_to_node_id[graph->num_rank_to_node_id++] = tmp_node->aligned_nodes_ids[a];
                        }
                    }
                } else {
                    node_marks[node_id] = 1;
                }
            }

            if (valid) {
                --stack_next_index; //pop();
            }
        }
    }

#ifdef CAUTIOUS_MODE
    assert(is_topologically_sorted(graph) == true);
#endif
}

bool is_topologically_sorted(po_graph *const graph) {
#ifdef CAUTIOUS_MODE
    assert(graph->num_nodes == graph->num_rank_to_node_id);
#endif

    uint32_t node_id;
    po_node *tmp_node;
    uint32_t e;

    bool *visited_nodes = calloc(graph->num_nodes, sizeof(bool));
    for (uint32_t i = 0; i < graph->num_nodes; ++i) {
        node_id = graph->rank_to_node_id[i];

        tmp_node = graph->nodes + node_id;
        for (e = 0; e < tmp_node->num_in_edges; ++e) {
            if (visited_nodes[tmp_node->in_edges[e]->begin_node_id] == false) {
                return false;
            }
        }

        visited_nodes[node_id] = true;
    }

    return true;
}

uint32_t initialize_multiple_sequence_alignment(
        po_graph *const graph,
        uint32_t **node_id_to_msa_rank) {
    uint32_t *_node_id_to_msa_rank = calloc(graph->num_nodes, sizeof(uint32_t));

    uint32_t node_id;
    uint32_t num_aligned_nodes_ids;

    uint32_t msa_id = 0;
    for (uint32_t i = 0; i < graph->num_nodes; ++i) {
        node_id = graph->rank_to_node_id[i];

        _node_id_to_msa_rank[node_id] = msa_id;

        num_aligned_nodes_ids = (graph->nodes + node_id)->num_aligned_nodes_ids;
        for (uint32_t j = 0; j < num_aligned_nodes_ids; ++j) {
            _node_id_to_msa_rank[graph->rank_to_node_id[++i]] = msa_id;
        }

        ++msa_id;
    }

    *node_id_to_msa_rank = _node_id_to_msa_rank;

    return msa_id;
}

void generate_multiple_sequence_alignment(
        po_graph *const graph,
        char ***msa_seq, uint32_t *msa_len,
        bool include_consensus) {

    // assign msa rank to each node
    uint32_t *node_id_to_msa_rank = NULL;
    *msa_len = initialize_multiple_sequence_alignment(graph, &node_id_to_msa_rank);

    uint32_t i, j;
    uint32_t node_id;

    uint32_t num_sequences = graph->num_sequences + include_consensus;

    char **_msa_seq = malloc(num_sequences * sizeof(char *));
    for (i = 0; i < num_sequences; ++i) {
        _msa_seq[i] = malloc(*msa_len * sizeof(char));
        for (j = 0; j < *msa_len; ++j) {
            _msa_seq[i][j] = '-';
        }
    }

    // extract sequences from graph and create msa strings (add indels(-) where necessary)
    for (i = 0; i < graph->num_sequences; ++i) {
        node_id = graph->sequences_begin_nodes_ids[i];

        while (true) {
            _msa_seq[i][node_id_to_msa_rank[node_id]] = (graph->nodes + node_id)->character;

            if (!po_node_successor(graph->nodes + node_id, &node_id, i)) {
                break;
            }
        }
    }

    if (include_consensus) {
        // do the same for consensus sequence
        po_graph_traverse_heaviest_bundle(graph);

        for (i = 0; i < graph->consensus_len; ++i) {
            node_id = graph->consensus[i];

            _msa_seq[graph->num_sequences][node_id_to_msa_rank[node_id]] = (graph->nodes + node_id)->character;
        }
    }

    *msa_seq = _msa_seq;
}

void reverse(uint32_t *arr, int n) {
    uint32_t aux[n];
    uint32_t i;

    for (i = 0; i < n; ++i) {
        aux[n - 1 - i] = arr[i];
    }

    for (i = 0; i < n; ++i) {
        arr[i] = aux[i];
    }
}

uint32_t po_graph_branch_completion(
        po_graph *const graph,
        int64_t *scores,
        int32_t *predecessors,
        uint32_t rank) {
    uint32_t i, j;

    uint32_t node_id = graph->rank_to_node_id[rank];
    po_node *tmp_node = graph->nodes + node_id;
    po_edge *tmp_edge;

    for (i = 0; i < tmp_node->num_out_edges; ++i) {
        tmp_edge = tmp_node->out_edges[i];

        for (j = 0; j < graph->nodes[tmp_edge->end_node_id].num_in_edges; ++j) {
            if ((graph->nodes + tmp_edge->end_node_id)->in_edges[j]->begin_node_id != node_id) {
                scores[(graph->nodes + tmp_edge->end_node_id)->in_edges[j]->begin_node_id] = -1;
            }
        }
    }

    int64_t max_score = 0;
    uint32_t max_score_id = 0;
    for (i = rank + 1; i < graph->num_nodes; ++i) {
        node_id = graph->rank_to_node_id[i];
        tmp_node = graph->nodes + node_id;

        scores[node_id] = -1;
        predecessors[node_id] = -1;

        for (j = 0; j < tmp_node->num_in_edges; j++) {
            if (scores[tmp_node->in_edges[j]->begin_node_id] == -1) {
                continue;
            }

            tmp_edge = tmp_node->in_edges[j];
            if (scores[node_id] < tmp_edge->total_weight ||
                (scores[node_id] == tmp_edge->total_weight &&
                 scores[predecessors[node_id]] <= scores[tmp_edge->begin_node_id]
                )) {
                scores[node_id] = tmp_edge->total_weight;
                predecessors[node_id] = tmp_edge->begin_node_id;
            }
        }

        if (predecessors[node_id] != -1) {
            scores[node_id] += scores[predecessors[node_id]];
        }

        if (max_score < scores[node_id]) {
            max_score = scores[node_id];
            max_score_id = node_id;
        }
    }

    return max_score_id;
}

void po_graph_traverse_heaviest_bundle(
        po_graph *const graph
) {
    uint32_t i, j;
    uint32_t node_id;
    uint32_t num_nodes = graph->num_nodes;
    po_node *tmp_node;
    po_edge *tmp_edge;

    int32_t *predecessors = malloc(num_nodes * sizeof(int32_t));
    int64_t *scores = malloc(num_nodes * sizeof(int64_t));
    for (i = 0; i < num_nodes; i++) {
        predecessors[i] = -1;
        scores[i] = -1;
    }

    uint32_t max_score_id = 0;
    for (i = 0; i < num_nodes; i++) {
        node_id = graph->rank_to_node_id[i];

        tmp_node = graph->nodes + node_id;
        for (j = 0; j < tmp_node->num_in_edges; j++) {
            tmp_edge = tmp_node->in_edges[j];

            if (scores[node_id] < tmp_edge->total_weight ||
                (scores[node_id] == tmp_edge->total_weight &&
                 scores[predecessors[node_id]] <= scores[tmp_edge->begin_node_id]
                )) {

                scores[node_id] = tmp_edge->total_weight;
                predecessors[node_id] = tmp_edge->begin_node_id;
            }
        }

        if (predecessors[node_id] != -1) {
            scores[node_id] += scores[predecessors[node_id]];
        }

        if (scores[max_score_id] < scores[node_id]) {
            max_score_id = node_id;
        }
    }

    if ((graph->nodes + max_score_id)->num_out_edges != 0) {
        uint32_t *node_id_to_rank = calloc(num_nodes, sizeof(uint32_t));

        for (i = 0; i < num_nodes; ++i) {
            node_id_to_rank[graph->rank_to_node_id[i]] = i;
        }

        do {
            max_score_id = po_graph_branch_completion(graph,
                                                      scores, predecessors,
                                                      node_id_to_rank[max_score_id]);
        } while ((graph->nodes + max_score_id)->num_out_edges != 0);
    }

    // traceback
    free(graph->consensus);
    graph->consensus = malloc(
            graph->num_nodes * sizeof(uint32_t)); //todo: manage in a clever way; graph->consensus.clear()
    graph->consensus_len = 0;

    while (predecessors[max_score_id] != -1) {
        graph->consensus[graph->consensus_len++] = max_score_id;
        max_score_id = predecessors[max_score_id];
    }
    graph->consensus[graph->consensus_len++] = max_score_id;

    reverse(graph->consensus, graph->consensus_len);
}

