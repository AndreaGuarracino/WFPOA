use crate::graph::{POGraph, Alignment};

mod graph;
mod wfpoa_edit_alignment;

use crate::wfpoa_edit_alignment::align_graph_to_seq;

mod edit_alignment;

use crate::edit_alignment::{edit_distance, edit_distance_on_po_graph};

// If the START/END nodes are in the POGraph
fn _shift_node_ids(alignment_result: &mut Alignment) {
    //for align_res in alignment_result.iter_mut() { if let Some(node_id) = align_res.0 { align_res.0 = Some(node_id + 2) } }
}

fn main() {
    let num_initial_sequences = 3;  // Number of sequences to align
    let num_initial_nodes = 128;    // At least equal to the length of the longest sequence

    let mut my_graph = POGraph::new(num_initial_sequences, num_initial_nodes);

    let mut alignment_result: Alignment = Vec::new();

    let seq1 = "AAAGGGAAA".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq1,
        weights.as_slice());

    let seq2 = "AAATTTAAA".as_bytes();
    alignment_result = Vec::with_capacity(9);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(6), Some(6)));
    alignment_result.push((Some(7), Some(7)));
    alignment_result.push((Some(8), Some(8)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq2,
        weights.as_slice());

    // Example 0
    /*
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
    _shift_node_ids(&mut alignment_result);
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
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq3,
        weights.as_slice());

    let seq4 = "TCGTTCC".as_bytes();
    alignment_result = Vec::with_capacity(6);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((None, Some(1)));
    alignment_result.push((Some(1), Some(2)));
    alignment_result.push((Some(9), Some(3)));
    alignment_result.push((Some(10), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(12), Some(6)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq4.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq4,
        weights.as_slice());
    */

    // Example 1
    /*
    let seq1 = "TCGAGT".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq1,
        weights.as_slice());

    let seq2 = "TCGACTAC".as_bytes();
    alignment_result = Vec::with_capacity(8);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((None, Some(6)));
    alignment_result.push((None, Some(7)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq2,
        weights.as_slice());

    let seq3 = "GCGGGTC".as_bytes();
    alignment_result = Vec::with_capacity(7);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(8), Some(6)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq3,
        weights.as_slice());

    let seq4 = "CAGTAC".as_bytes();
    alignment_result = Vec::with_capacity(6);
    alignment_result.push((Some(1), Some(0)));
    alignment_result.push((Some(3), Some(1)));
    alignment_result.push((Some(4), Some(2)));
    alignment_result.push((Some(5), Some(3)));
    alignment_result.push((Some(7), Some(4)));
    alignment_result.push((Some(8), Some(5)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq4.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq4,
        weights.as_slice());
    */

    // Example 2
    /*
    let seq1 = "ATCTA".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq1,
        weights.as_slice());

    let seq2 = "CTCGA".as_bytes();
    alignment_result = Vec::with_capacity(5);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq2,
        weights.as_slice());

    let seq3 = "GCTA".as_bytes();
    alignment_result = Vec::with_capacity(4);
    alignment_result.push((Some(1), Some(0)));
    alignment_result.push((Some(2), Some(1)));
    alignment_result.push((Some(3), Some(2)));
    alignment_result.push((Some(4), Some(3)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq3,
        weights.as_slice());
    */

    // Example 3
    /*
    let seq1 = "CAAATAAGT".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq1,
        weights.as_slice());

    let seq2 = "CCAATAAT".as_bytes();
    //0 (C)   0 (C) <---
    //1 (A)   1 (C) <---
    //2 (A)   2 (A) <---
    //3 (A)   3 (A) <---
    //4 (T)   4 (T) <---
    //5 (A)   5 (A) <---
    //6 (A)   6 (A) <---
    //7 (G)   -1
    //8 (T)   7 (T) <---
    alignment_result = Vec::with_capacity(9);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(1), Some(1)));
    alignment_result.push((Some(2), Some(2)));
    alignment_result.push((Some(3), Some(3)));
    alignment_result.push((Some(4), Some(4)));
    alignment_result.push((Some(5), Some(5)));
    alignment_result.push((Some(6), Some(6)));
    alignment_result.push((Some(7), None));
    alignment_result.push((Some(8), Some(7)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq2,
        weights.as_slice());

    let seq3 = "CCTATC".as_bytes();
    //0 (C)   0 (C) <---
    //9 (C)   1 (C) <---
    //2 (A)   -1
    //3 (A)   -1
    //4 (T)   2 (T) <---
    //5 (A)   3 (A) <---
    //6 (A)   4 (T) <---
    //8 (T)   5 (C) <---
    alignment_result = Vec::with_capacity(9);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(9), Some(1)));
    alignment_result.push((Some(2), None));
    alignment_result.push((Some(3), None));
    alignment_result.push((Some(4), Some(2)));
    alignment_result.push((Some(5), Some(3)));
    alignment_result.push((Some(6), Some(4)));
    alignment_result.push((Some(8), Some(5)));
    _shift_node_ids(&mut alignment_result);
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(
        &alignment_result,
        seq3,
        weights.as_slice());
    */

    //print!("{}\n", my_graph);

    let (_msa_len, msa_seq) = my_graph.generate_multiple_sequence_alignment(&true);
    for aligned_seq in msa_seq.iter() {
        print!("{}\n", String::from_utf8_lossy(&aligned_seq));
    }

    //println!("{}", String::from_utf8(my_graph.graph_sequence()).unwrap());

    //my_graph.graph_2_dot().expect("graph_2_dot: file write error");
    // dot WFPOA_graph.dot -T png | display

    //edit_distance("AAAGGGAAA".as_bytes(), "AAATATATA".as_bytes());
    edit_distance_on_po_graph(&my_graph, "AAATATATA".as_bytes());

    //let seq = "AAATATATA".as_bytes();
    //wfpoa_edit_alignment::align_graph_to_seq(&my_graph, seq);
}
