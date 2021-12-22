use crate::edit_alignment::edit_distance_on_po_graph;
use crate::graph::{Alignment, POGraph};

mod graph;
mod edit_alignment;

fn main() {
    let num_initial_sequences = 3; // Number of sequences to align
    let num_initial_nodes = 128; // At least equal to the length of the longest sequence

    let mut my_graph = POGraph::new(num_initial_sequences, num_initial_nodes);

    let mut alignment_result: Alignment = Vec::new();

    // Example
    let seq1 = "CAAATAAGT".as_bytes();
    //let seq1 = "TGAAACAT".as_bytes();
    let weights = vec![1; seq1.len()];
    my_graph.add_alignment(&alignment_result, seq1, weights.as_slice());

    let seq2 = "CCAATAAT".as_bytes();
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
    // let seq2 = "CGTTTCC".as_bytes();
    // alignment_result = Vec::with_capacity(7);
    // alignment_result.push((Some(0), Some(0)));
    // alignment_result.push((Some(1), Some(1)));
    // alignment_result.push((Some(2), Some(2)));
    // alignment_result.push((Some(3), Some(3)));
    // alignment_result.push((Some(4), Some(4)));
    // alignment_result.push((Some(5), Some(5)));
    // alignment_result.push((Some(6), Some(6)));
    let weights = vec![1; seq2.len()];
    my_graph.add_alignment(&alignment_result, seq2, weights.as_slice());

    let seq3 = "CCTATC".as_bytes();
    alignment_result = Vec::with_capacity(8);
    alignment_result.push((Some(0), Some(0)));
    alignment_result.push((Some(9), Some(1)));
    alignment_result.push((Some(2), None));
    alignment_result.push((Some(3), None));
    alignment_result.push((Some(4), Some(2)));
    alignment_result.push((Some(5), Some(3)));
    alignment_result.push((Some(6), Some(4)));
    alignment_result.push((Some(8), Some(5)));
    // let seq3 = "AGTGTCC".as_bytes();
    // alignment_result = Vec::with_capacity(7);
    // alignment_result.push((Some(0), Some(0)));
    // alignment_result.push((Some(1), Some(1)));
    // alignment_result.push((Some(2), Some(2)));
    // alignment_result.push((Some(3), Some(3)));
    // alignment_result.push((Some(4), Some(4)));
    // alignment_result.push((Some(5), Some(5)));
    // alignment_result.push((Some(6), Some(6)));
    let weights = vec![1; seq3.len()];
    my_graph.add_alignment(&alignment_result, seq3, weights.as_slice());

    println!("{}", my_graph);

    let (_msa_len, msa_seq) = my_graph.generate_multiple_sequence_alignment(&true);
    for aligned_seq in msa_seq.iter() {
        println!("{}", String::from_utf8_lossy(&aligned_seq));
    }

    //println!("{}", String::from_utf8(my_graph.graph_sequence()).unwrap());

    //my_graph.graph_2_dot().expect("graph_2_dot: file write error");
    // dot WFPOA_graph.dot -T png | display

    //edit_distance("AAAGGGAAA".as_bytes(), "AAATATATA".as_bytes());
    edit_distance_on_po_graph(&my_graph, "AAATATATA".as_bytes());
}
