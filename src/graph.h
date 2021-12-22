#ifndef WFPOA_GRAPH_H
#define WFPOA_GRAPH_H

#include <inttypes.h>
#include <z3.h>

typedef struct {
    int32_t first;
    int32_t second;
} pair;

typedef struct {
    pair *aligned_pairs;
    //uint32_t num_allocated_pairs;
    uint32_t num_pairs;
} alignment;

typedef struct {
    uint32_t begin_node_id;
    uint32_t end_node_id;

    uint32_t *sequence_labels; //sequence_rank
    //uint32_t num_allocated_sequence_labels;
    uint32_t num_sequence_labels;

    int64_t total_weight;
} po_edge;

typedef struct {
    uint32_t id;
    char character; //std::uint32_t code_;

    po_edge **in_edges;
    //uint32_t num_allocated_in_edges;
    uint32_t num_in_edges;

    po_edge **out_edges;
    //uint32_t num_allocated_out_edges;
    uint32_t num_out_edges;

    uint32_t *aligned_nodes_ids;
    //uint32_t num_allocated_aligned_nodes_ids;
    uint32_t num_aligned_nodes_ids;
} po_node;

typedef struct {
    uint32_t num_sequences;

    po_node *nodes;
    //uint32_t num_allocated_nodes;
    uint32_t num_nodes;

    uint32_t *rank_to_node_id;
    //uint32_t num_allocated_rank_to_node_id;
    uint32_t num_rank_to_node_id;

    uint32_t *sequences_begin_nodes_ids;

    uint32_t *consensus;
    uint32_t consensus_len;
} po_graph;

bool po_node_successor(
        po_node *node,
        uint32_t *node_id,
        uint32_t label);

void po_edge_add_sequence(
        po_edge *edge,
        uint32_t label,
        uint32_t weight
);

void po_graph_init(
        po_graph *graph,
        uint32_t num_initial_nodes
);

void po_graph_create_and_init_node(
        po_node *node,
        uint32_t id,
        char character
);

uint32_t po_graph_add_node(
        po_graph *graph,
        char character);

void po_graph_add_edge(
        po_graph *wf_graph,
        uint32_t begin_node_id, uint32_t end_node_id,
        uint32_t weight);

int64_t po_graph_add_sequence(
        po_graph *wf_graph,
        const char *sequence,
        const uint32_t *weights,
        uint32_t begin, uint32_t end);

void po_graph_add_alignment(
        po_graph *wf_graph,
        const alignment *alignment,
        const char *sequence, uint32_t sequence_size,
        const uint32_t *weights);

void topological_sort(po_graph *graph);

bool is_topologically_sorted(po_graph *graph);

uint32_t initialize_multiple_sequence_alignment(
        po_graph *graph,
        uint32_t **node_id_to_msa_column);

void generate_multiple_sequence_alignment(
        po_graph *graph,
        char ***msa_seq, uint32_t *msa_len,
        bool include_consensus);

uint32_t po_graph_branch_completion(
        po_graph *graph,
        int64_t *scores,
        int64_t *predecessors,
        uint32_t rank);

void po_graph_traverse_heaviest_bundle(
        po_graph *graph
);

#endif //WFPOA_GRAPH_H
