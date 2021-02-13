
use crate::graph::{POGraph, Alignment};

mod graph;

fn main() {
    let num_initial_sequences = 3;  // Number of sequences to align	
    let num_initial_nodes = 128;    // At least equal to the length of the longest sequence	

    let mut my_graph = POGraph::new(num_initial_sequences, num_initial_nodes);

    let mut alignment_result: Alignment = Vec::new();

    // Example	
    let seq1 = "TGAAACAT".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq1,
        weights.as_slice());

    let seq2 = "CGTTTCC".as_bytes();
    alignment_result = Vec::with_capacity(7);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(6), Some(6)));
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq2,
        weights.as_slice());

    let seq3 = "AGTGTCC".as_bytes();
    alignment_result = Vec::with_capacity(7);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(6), Some(6)));
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq3,
        weights.as_slice());

    print!("{}\n", my_graph);

    let (_msa_len, msa_seq) = my_graph.generate_multiple_sequence_alignment(&true);
    for aligned_seq in msa_seq.iter() {
        print!("{}\n", String::from_utf8_lossy(&aligned_seq));
    }
}