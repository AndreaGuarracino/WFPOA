use crate::graph::{POGraph, Alignment};

mod graph;

fn main() {
    let num_initial_sequences = 3;  // Number of sequences to align
    let num_initial_nodes = 128;    // At least equal to the length of the longest sequence

    let mut my_graph = POGraph::new(num_initial_sequences, num_initial_nodes);

    let mut alignment_result: Alignment = Vec::new();

    let seq1 = "CAAATAAGT".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(
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
    alignment_result = Vec::with_capacity(9);
    alignment_result.push((Some(2), Some(0)));
    alignment_result.push((Some(3), Some(1)));
    alignment_result.push((Some(4), Some(2)));
    alignment_result.push((Some(5), Some(3)));
    alignment_result.push((Some(6), Some(4)));
    alignment_result.push((Some(7), Some(5)));
    alignment_result.push((Some(8), Some(6)));
    alignment_result.push((Some(9), None));
    alignment_result.push((Some(10), Some(7)));
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(
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
    alignment_result = Vec::with_capacity(9);
    alignment_result.push((Some(2), Some(0)));
    alignment_result.push((Some(11), Some(1)));
    alignment_result.push((Some(4), None));
    alignment_result.push((Some(5), None));
    alignment_result.push((Some(6), Some(2)));
    alignment_result.push((Some(7), None));
    alignment_result.push((Some(8), Some(3)));
    alignment_result.push((Some(10), Some(4)));
    alignment_result.push((None, Some(5)));
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq3,
        weights.as_slice());

    print!("{}\n", my_graph);

    let (_msa_len, msa_seq) = my_graph.generate_multiple_sequence_alignment(&true);
    for aligned_seq in msa_seq.iter() {
        print!("{}\n", String::from_utf8_lossy(aligned_seq));
    }
}
