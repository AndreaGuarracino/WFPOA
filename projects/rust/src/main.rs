use crate::graph::{POGraph, po_graph_init, Alignment, po_graph_add_alignment, generate_multiple_sequence_alignment};
use std::borrow::Borrow;

mod graph;

fn main() {
    let num_initial_nodes = 20;
    let num_initial_sequences = 3;
    let mut my_graph: graph::POGraph = POGraph {
        nodes: Vec::with_capacity(num_initial_nodes),
        rank_to_node_id: Vec::with_capacity(num_initial_nodes),
        sequences_begin_nodes_ids: Vec::with_capacity(num_initial_sequences),
        consensus: Vec::with_capacity(num_initial_nodes),
    };

    //po_graph_init(&mut my_graph, 20, 3);

    let mut alignment_result: Alignment = Vec::new();

    let seq1 = "CAAATAAGT".as_bytes();
    let weights = vec![1; seq1.len()];
    po_graph_add_alignment(
        &mut my_graph,
        &alignment_result,
        seq1,
        weights.as_slice());

    let seq2 = "CCAATAAT".as_bytes();
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
    let mut alignment_result: Alignment = Vec::with_capacity(9);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(6), Some(6)));
    alignment_result.push((Some(7), None));
    alignment_result.push((Some(8), Some(7)));
    let weights = vec![1; seq2.len()];
    po_graph_add_alignment(
        &mut my_graph,
        &alignment_result,
        seq2,
        weights.as_slice());

    let seq3 = "CCTATC".as_bytes();
    /*
    0 (C)   0 (C) <---
    9 (C)   1 (C) <---
    2 (A)   -1
    3 (A)   -1
    4 (T)   2 (T) <---
    5 (A)   -1
    6 (A)   3 (A) <---
    8 (T)   4 (T) <---
    -1      5 (C) <---
    */
    let mut alignment_result: Alignment = Vec::with_capacity(9);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(9), Some(1)));
    alignment_result.push((Some(2), None));
    alignment_result.push((Some(3), None));
    alignment_result.push((Some(4), Some(2)));
    alignment_result.push((Some(5), None));
    alignment_result.push((Some(6), Some(3)));
    alignment_result.push((Some(8), Some(4)));
    alignment_result.push((None, Some(5)));
    let weights = vec![1; seq3.len()];
    po_graph_add_alignment(
        &mut my_graph,
        &alignment_result,
        seq3,
        weights.as_slice());

    print!("num_sequences: {}\n", my_graph.sequences_begin_nodes_ids.len());
    print!("num_nodes {}\n", my_graph.nodes.len());
    for node in &my_graph.nodes {
        print!("\t{}: {}\n", node.id, node.character as char);
        for edge in &node.in_edges {
            print!("\t\t{} <--- {}\n", edge.end_node_id, edge.begin_node_id);
            for label in &edge.sequence_labels {
                print!("\t\t\tsequence_label: {}\n", label);
            }
        }

        for edge in &node.out_edges {
            print!("\t\t{} ---> {}\n", edge.begin_node_id, edge.end_node_id);
            for label in &edge.sequence_labels {
                print!("\t\t\tsequence_label: {}\n", label);
            }
        }
    }

    print!("Rank\tNodeId\n");
    for i_node in my_graph.rank_to_node_id.iter().enumerate() {
        print!("\t{}\t{}\n", i_node.0, i_node.1);
    }

    let (msa_len, msa_seq) = generate_multiple_sequence_alignment(&mut my_graph, &true);
    for aligned_seq in msa_seq.iter() {
        print!("{}\n", String::from_utf8_lossy(aligned_seq));
    }
}
