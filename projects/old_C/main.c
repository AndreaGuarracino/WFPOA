#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "graph.h"


int main(int argc, char *argv[]) {
    po_graph graph;
    po_graph_init(&graph, 20);

    alignment alignment_result;

    uint32_t *weights = malloc(20 * sizeof(uint32_t));
    for (uint8_t i = 0; i < 20; i++) { weights[i] = 1; }

    {
        char *seq1 = "CAAATAAGT";
        alignment_result.num_pairs = 0;

        po_graph_add_alignment(
                &graph,
                &alignment_result,
                seq1, strlen(seq1),
                weights);
    }

    {
        char *seq2 = "CCAATAAT";
        /*
        0 (C)   0 (C) <---
        1 (A)   1 (C) <---
        2 (A)   2 (A) <---
        3 (A)   3 (A) <---
        4 (T)   4 (T) <---
        5 (A)   5 (A) <---
        6 (A)   6 (A) <---
        7 (G)   -1
        8 (T)   7 (T) <---
        */
        alignment_result.num_pairs = 9;
        alignment_result.aligned_pairs = malloc(alignment_result.num_pairs * sizeof(pair));
        alignment_result.aligned_pairs[0].first = 0;
        alignment_result.aligned_pairs[0].second = 0;
        alignment_result.aligned_pairs[1].first = 1;
        alignment_result.aligned_pairs[1].second = 1;
        alignment_result.aligned_pairs[2].first = 2;
        alignment_result.aligned_pairs[2].second = 2;
        alignment_result.aligned_pairs[3].first = 3;
        alignment_result.aligned_pairs[3].second = 3;
        alignment_result.aligned_pairs[4].first = 4;
        alignment_result.aligned_pairs[4].second = 4;
        alignment_result.aligned_pairs[5].first = 5;
        alignment_result.aligned_pairs[5].second = 5;
        alignment_result.aligned_pairs[6].first = 6;
        alignment_result.aligned_pairs[6].second = 6;
        alignment_result.aligned_pairs[7].first = 7;
        alignment_result.aligned_pairs[7].second = -1;
        alignment_result.aligned_pairs[8].first = 8;
        alignment_result.aligned_pairs[8].second = 7;

        po_graph_add_alignment(
                &graph,
                &alignment_result,
                seq2, strlen(seq2),
                weights);
    }

    {
        char *seq3 = "CCTATC";
        /*
        0 (C)   0 (C) <---
        9 (C)   1 (C) <---
        2 (A)   -1
        3 (A)   -1
        4 (T)   2 (T) <---
        5 (A)   -1
        6 (A)   3 (A) <---
        8 (T)   4 (T) <---
        -1       5 (C) <---
        */
        alignment_result.num_pairs = 8;
        alignment_result.aligned_pairs = malloc(alignment_result.num_pairs * sizeof(pair));
        alignment_result.aligned_pairs[0].first = 0;
        alignment_result.aligned_pairs[0].second = 0;
        alignment_result.aligned_pairs[1].first = 9;
        alignment_result.aligned_pairs[1].second = 1;
        alignment_result.aligned_pairs[2].first = 2;
        alignment_result.aligned_pairs[2].second = -1;
        alignment_result.aligned_pairs[3].first = 3;
        alignment_result.aligned_pairs[3].second = -1;
        alignment_result.aligned_pairs[4].first = 4;
        alignment_result.aligned_pairs[4].second = 2;
        alignment_result.aligned_pairs[5].first = 5;
        alignment_result.aligned_pairs[5].second = 3;
        alignment_result.aligned_pairs[6].first = 6;
        alignment_result.aligned_pairs[6].second = 4;
        alignment_result.aligned_pairs[7].first = 8;
        alignment_result.aligned_pairs[7].second = 5;

        po_graph_add_alignment(
                &graph,
                &alignment_result,
                seq3, strlen(seq3),
                weights);
    }

    po_node *tmp_node;
    po_edge *tmp_edge;
    printf("num_sequences: %d\n", graph.num_sequences);
    printf("num_nodes %d\n", graph.num_nodes);
    for (uint32_t i = 0; i < graph.num_nodes; i++) {
        tmp_node = graph.nodes + i;
        printf("\t%d: %c\n", tmp_node->id, tmp_node->character);
        for (uint32_t j = 0; j < tmp_node->num_in_edges; j++) {
            tmp_edge = tmp_node->in_edges[j];
            printf(
                    "\t\t%d (%c) <--- %d (%c) - W: %ld\n",
                    tmp_edge->end_node_id,
                    graph.nodes[tmp_edge->end_node_id].character,
                    tmp_edge->begin_node_id,
                    graph.nodes[tmp_edge->begin_node_id].character,
                    tmp_edge->total_weight
            );
            for (uint32_t z = 0; z < tmp_edge->num_sequence_labels; z++) {
                printf("\t\t\tsequence_label: %d\n", tmp_edge->sequence_labels[z]);
            }
        }

        for (uint32_t j = 0; j < tmp_node->num_out_edges; j++) {
            tmp_edge = tmp_node->out_edges[j];
            printf(
                    "\t\t%d (%c) ---> %d (%c) - W: %ld\n",
                    tmp_edge->begin_node_id,
                    graph.nodes[tmp_edge->begin_node_id].character,
                    tmp_edge->end_node_id,
                    graph.nodes[tmp_edge->end_node_id].character,
                    tmp_edge->total_weight
            );
            for (uint32_t z = 0; z < tmp_edge->num_sequence_labels; z++) {
                printf("\t\t\tsequence_label: %d\n", tmp_edge->sequence_labels[z]);
            }
        }
    }

    printf("Rank\tNodeId\n");
    for (uint32_t i = 0; i < graph.num_nodes; i++) {
        printf("\t%d\t%d (%c)\n", i, graph.rank_to_node_id[i], graph.nodes[graph.rank_to_node_id[i]].character);
    }

    char **msa_seq = NULL;
    uint32_t msa_len;
    generate_multiple_sequence_alignment(&graph, &msa_seq, &msa_len, true);

    for (uint32_t i = 0; i < graph.num_sequences + 1; i++) {
        printf("%s\n", msa_seq[i]);
    }

    po_graph_to_dot(&graph);
    // dot WFPOA_graph.dot -T png | display
}
