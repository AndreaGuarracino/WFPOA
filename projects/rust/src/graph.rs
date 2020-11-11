pub(crate) type Alignment = Vec<(Option<usize>, Option<usize>)>;

// todo I want to avoid cloning edges (I need pointer to manage two owner)
#[derive(Clone)]
// todo remove pub and implement a trait for having a to_string for the nodes
pub struct POEdge {
    pub(crate) begin_node_id: usize,
    pub(crate) end_node_id: usize,

    pub(crate) sequence_labels: Vec<u32>,

    total_weight: i64,
}

// todo remove pub and implement a trait for having a to_string for the nodes
pub struct PONode {
    pub(crate) id: usize,
    pub(crate) character: u8,

    pub(crate) in_edges: Vec<POEdge>,
    pub(crate) out_edges: Vec<POEdge>,

    aligned_nodes_ids: Vec<usize>,
}

pub struct POGraph {
    pub(crate) nodes: Vec<PONode>,

    pub(crate) rank_to_node_id: Vec<usize>,

    pub(crate) sequences_begin_nodes_ids: Vec<usize>,

    pub(crate) consensus: Vec<usize>,
}

fn po_node_successor(
    node: &PONode,
    label: &u32,
) -> Option<usize> {
    for edge in node.out_edges.iter() {
        for current_label in edge.sequence_labels.iter() {
            if current_label == label {
                return Some(edge.end_node_id);
            }
        }
    }

    return None;
}

/*fn po_node_successor(
    node : &PONode,
    node_id : &mut usize,
    label : u32
) -> bool {
    for edge in node.out_edges {
        for l in edge.sequence_labels {
            if l == label {
                *node_id = edge.end_node_id;
                return true;
            }
        }
    }

    return false;
}*/

fn po_edge_add_sequence(
    edge: &mut POEdge,
    label: u32,
    weight: u32) {
    edge.sequence_labels.push(label);
    edge.total_weight += weight as i64;
}

pub fn po_graph_init(
    graph: &mut POGraph,
    num_initial_nodes: usize,
    num_initial_sequences: usize,
) {
    graph.nodes = Vec::with_capacity(num_initial_nodes);
    graph.rank_to_node_id = Vec::with_capacity(num_initial_nodes);

    graph.sequences_begin_nodes_ids = Vec::with_capacity(num_initial_sequences);

    graph.consensus = Vec::with_capacity(num_initial_nodes);
}

/*fn po_graph_create_and_int_edge(
    edge: &mut POEdge,
    begin_node_id: usize,
    end_node_id: usize,
    label: u32,
    weight: u32,
) {
    edge.begin_node_id = begin_node_id;
    edge.end_node_id = end_node_id;

    edge.sequence_labels = vec![label; 1];

    edge.total_weight = weight as i64;
}*/

fn po_graph_add_node(
    graph: &mut POGraph,
    character: u8,
) -> usize {
    // po_graph_create_and_init_node()
    graph.nodes.push(PONode {
        id: graph.nodes.len(),
        character,
        in_edges: vec![],
        out_edges: vec![],
        aligned_nodes_ids: vec![],
    });

    return graph.nodes.last().unwrap().id;
}

fn po_graph_add_edge(
    graph: &mut POGraph,
    begin_node_id: usize,
    end_node_id: usize,
    weight: u32,
) {
    for edge in graph.nodes[begin_node_id].out_edges.iter_mut() {
        if edge.end_node_id == end_node_id {
            po_edge_add_sequence(edge, graph.sequences_begin_nodes_ids.len() as u32, weight);

            //+++++++++++++++++++++++++++++++++++
            // todo I need a way to manage that without the shared_ptr (not available in Rust!)
            //  with shared pointer I would not need to update both the memory locations
            for edge in graph.nodes[end_node_id].in_edges.iter_mut() {
                if edge.begin_node_id == begin_node_id {
                    po_edge_add_sequence(edge, graph.sequences_begin_nodes_ids.len() as u32, weight);
                    return;
                }
            }
            panic!("po_graph_add_edge: edge that was supposed to be present not found!");
            //+++++++++++++++++++++++++++++++++++
        }
    }

    // po_graph_create_and_int_edge()
    let new_edge: POEdge = POEdge {
        begin_node_id,
        end_node_id,
        sequence_labels: vec![graph.sequences_begin_nodes_ids.len() as u32],
        total_weight: weight as i64,
    };

    // todo manage two owners, avoiding duplicating stuff
    graph.nodes[begin_node_id].out_edges.push(new_edge.clone());
    graph.nodes[end_node_id].in_edges.push(new_edge.clone());
}

fn po_graph_add_sequence(
    graph: &mut POGraph,
    sequence: &[u8],
    weights: &[u32],
    begin: usize,
    end: usize) -> Option<usize> {
    if begin == end {
        return None;
    }

    let first_node_id = po_graph_add_node(graph, sequence[begin]);
    for i in (begin + 1)..end {
        let node_id = po_graph_add_node(graph, sequence[i]);

        // both nodes contribute to edge weight
        po_graph_add_edge(graph, node_id - 1, node_id, weights[i - 1] + weights[i]);
    }

    return Some(first_node_id);
}

pub fn po_graph_add_alignment(
    graph: &mut POGraph,
    alignment: &Alignment,
    sequence: &[u8],
    weights: &[u32],
) {
    //#ifdef CAUTIOUS_MODE
    if sequence.len() == 0 {
        return;
    }

    if sequence.len() != weights.len() {
        panic!("[wfpoa::po_graph_add_alignment] error: \
        sequence and weights are of unequal size!");
    }
    //#endif

    let mut begin_node_id;
    if alignment.is_empty() { //  no alignment
        begin_node_id = po_graph_add_sequence(graph, sequence, weights, 0, sequence.len());
    } else {
        let mut valid_seq_ids: Vec<usize> = Vec::with_capacity(alignment.len());
        for align in alignment.iter() {
            if let Some(seq_id) = align.1 {
                valid_seq_ids.push(seq_id);
            }
        }

        //#ifdef CAUTIOUS_MODE
        assert!(valid_seq_ids[0] <= sequence.len());
        assert!(*valid_seq_ids.last().unwrap() + 1 <= sequence.len());
        //#endif

        let tmp = graph.nodes.len();
        begin_node_id = po_graph_add_sequence(graph, sequence, weights, 0, valid_seq_ids[0]);
        let mut head_node_id = if tmp == graph.nodes.len() { None } else { Some(graph.nodes.len() - 1) };

        let tail_node_id = po_graph_add_sequence(
            graph, sequence, weights,
            *valid_seq_ids.last().unwrap() + 1, sequence.len(),
        );

        let mut new_node_id: Option<usize>;
        let mut prev_weight = match head_node_id {
            Some(_) => weights[valid_seq_ids[0] - 1],
            None => 0,
        };

        for (node_id, seq_id) in alignment.iter() {
            if let Some(seq_id) = seq_id {
                let letter = sequence[*seq_id];
                if let Some(node_id) = node_id {
                    if graph.nodes[*node_id].character == letter {
                        new_node_id = Some(*node_id);
                    } else {
                        let mut aligned_to_node_id: Option<usize> = None;
                        for aligned_node_id in graph.nodes[*node_id].aligned_nodes_ids.iter() {
                            if graph.nodes[*aligned_node_id].character == letter {
                                aligned_to_node_id = Some(*aligned_node_id);
                                break;
                            }
                        }

                        if aligned_to_node_id == None {
                            new_node_id = Some(po_graph_add_node(graph, letter));

                            // todo: I don't like to copy everything to avoid borrowing problem in the for
                            let aligned_nodes_ids = graph.nodes[*node_id].aligned_nodes_ids.to_vec();

                            for aligned_node_id in aligned_nodes_ids.iter() {
                                graph.nodes[new_node_id.unwrap()].aligned_nodes_ids.push(*aligned_node_id);
                                graph.nodes[*aligned_node_id].aligned_nodes_ids.push(new_node_id.unwrap());
                            }

                            graph.nodes[new_node_id.unwrap()].aligned_nodes_ids.push(*node_id);
                            graph.nodes[*node_id].aligned_nodes_ids.push(new_node_id.unwrap());
                        } else {
                            new_node_id = aligned_to_node_id;
                        }
                    }
                } else {
                    new_node_id = Some(po_graph_add_node(graph, letter));
                }

                if begin_node_id == None {
                    begin_node_id = new_node_id;
                }

                if let Some(head_node_id) = head_node_id {
                    // both nodes contribute to edge weight
                    po_graph_add_edge(graph,
                                      head_node_id, new_node_id.unwrap(),
                                      prev_weight + weights[*seq_id],
                    );
                }

                head_node_id = new_node_id;
                prev_weight = weights[*seq_id];
            }
        }

        if let Some(tail_node_id) = tail_node_id {
            // both nodes contribute to edge weight
            po_graph_add_edge(graph,
                              head_node_id.unwrap(), tail_node_id,
                              prev_weight + weights[*valid_seq_ids.last().unwrap() + 1],
            );
        }
    }

    graph.sequences_begin_nodes_ids.push(begin_node_id.unwrap());

    topological_sort(graph);
}

fn topological_sort(
    graph: &mut POGraph
) {
    graph.rank_to_node_id.clear();
    graph.rank_to_node_id = Vec::with_capacity(graph.nodes.len());

    // 0 - unmarked, 1 - temporarily marked, 2 - permanently marked
    let mut node_marks: Vec<u8> = vec![0; graph.nodes.len()];
    let mut do_not_check_aligned_nodes: Vec<bool> = vec![false; graph.nodes.len()];
    let mut nodes_to_visit: Vec<usize> = Vec::new();

    for i in 0..graph.nodes.len() {
        if node_marks[i] != 0 {
            continue;
        }

        nodes_to_visit.push(i);
        while !nodes_to_visit.is_empty() {
            let node_id = nodes_to_visit.last().unwrap().clone();

            let mut valid = true;

            if node_marks[node_id] != 2 {
                for edge in &graph.nodes[node_id].in_edges {
                    if node_marks[edge.begin_node_id] != 2 {
                        nodes_to_visit.push(edge.begin_node_id);
                        valid = false;
                    }
                }

                if !do_not_check_aligned_nodes[node_id] {
                    for aligned_node_id in &graph.nodes[node_id].aligned_nodes_ids {
                        if node_marks[*aligned_node_id] != 2 {
                            nodes_to_visit.push(*aligned_node_id);
                            do_not_check_aligned_nodes[*aligned_node_id] = true;
                            valid = false;
                        }
                    }
                }

                //#ifdef CAUTIOUS_MODE
                //    assert!((valid || node_marks[node_id] != 1) && "Graph is not a DAG!");
                assert!(valid || node_marks[node_id] != 1, "Graph is not a DAG!");
                //#endif

                if valid {
                    node_marks[node_id] = 2;
                    if !do_not_check_aligned_nodes[node_id] {
                        graph.rank_to_node_id.push(node_id);
                        graph.rank_to_node_id.extend(graph.nodes[node_id].aligned_nodes_ids.to_vec());
                    }
                } else {
                    node_marks[node_id] = 1;
                }
            }

            if valid {
                nodes_to_visit.pop();
            }
        }
    }

    //#ifdef CAUTIOUS_MODE
    assert!(is_topologically_sorted(graph) == true);
    //#endif
}

fn is_topologically_sorted(
    graph: &mut POGraph
) -> bool {
    //#ifdef CAUTIOUS_MODE
    assert_eq!(graph.nodes.len(), graph.rank_to_node_id.len());
    //#endif

    let mut visited_nodes = vec![false; graph.nodes.len()];

    for i in 0..graph.nodes.len() {
        let node_id = &graph.rank_to_node_id[i];

        for edge in &graph.nodes[*node_id].in_edges {
            if !visited_nodes[edge.begin_node_id] {
                return false;
            }
        }

        visited_nodes[*node_id] = true;
    }

    return true;
}

fn initialize_multiple_sequence_alignment(
    graph: &POGraph,
) -> (usize, Vec<usize>) {
    let mut node_id_to_msa_rank = vec![0; graph.nodes.len()];

    let mut msa_id: usize = 0;
    let mut i = 0;
    loop {
        let node_id = graph.rank_to_node_id[i];

        node_id_to_msa_rank[node_id] = msa_id.clone();

        for _ in 0..graph.nodes[node_id].aligned_nodes_ids.len() {
            i += 1;
            node_id_to_msa_rank[graph.rank_to_node_id[i]] = msa_id.clone();
        }

        msa_id += 1;

        i += 1;
        if i >= graph.nodes.len(){
            break;
        }
    }

    return (msa_id, node_id_to_msa_rank);
}

pub fn generate_multiple_sequence_alignment(
    graph: &mut POGraph,
    include_consensus: &bool,
) -> (usize, Vec<Vec<u8>>) {
    let (msa_len, node_id_to_msa_rank) = initialize_multiple_sequence_alignment(graph);

    let num_sequences = graph.sequences_begin_nodes_ids.len() + if *include_consensus { 1 } else { 0 };

    let mut msa_seq = vec![vec![b'-'; msa_len]; num_sequences];
    // extract sequences from graph and create msa strings (add indels(-) where necessary)

    for i in 0..graph.sequences_begin_nodes_ids.len() {
        let mut node_id = graph.sequences_begin_nodes_ids[i];

        loop {
            msa_seq[i][node_id_to_msa_rank[node_id]] = graph.nodes[node_id].character.clone();

            match po_node_successor(&graph.nodes[node_id], &(i as u32)) {
                Some(new_node_id) => node_id = new_node_id,
                None => break
            }
        }
    }

    if *include_consensus {
        // do the same for consensus sequence
        po_graph_traverse_heaviest_bundle(graph);

        for node_id in &graph.consensus {
            msa_seq[num_sequences - 1][node_id_to_msa_rank[*node_id]] = graph.nodes[*node_id].character.clone();
        }
    }

    return (msa_len, msa_seq);
}

fn po_graph_branch_completion(
    graph: &POGraph,
    scores: &mut Vec<i64>,
    predecessors: &mut Vec<i32>,
    rank: &usize,
) -> usize {
    let node_id = graph.rank_to_node_id[*rank].clone();

    for out_edge in &graph.nodes[node_id].out_edges {
        for in_edge in &graph.nodes[out_edge.end_node_id].in_edges {
            if in_edge.begin_node_id != node_id {
                scores[out_edge.begin_node_id] = -1;
            }
        }
    }

    let mut max_score: i64 = 0;
    let mut max_score_id: u32 = 0;
    for i in (rank + 1)..graph.nodes.len() {
        let node_id = graph.rank_to_node_id[i].clone();
        let node = &graph.nodes[node_id];

        scores[node_id] = -1;
        predecessors[node_id] = -1;

        for edge in &node.in_edges {
            if scores[edge.begin_node_id] == -1 {
                continue;
            }

            if scores[node_id] < edge.total_weight ||
                (scores[node_id] == edge.total_weight &&
                    scores[predecessors[node_id] as usize] <= scores[edge.begin_node_id]
                ) {
                scores[node_id] = edge.total_weight;
                predecessors[node_id] = edge.begin_node_id as i32;
            }
        }

        if predecessors[node_id] != -1 {
            scores[node_id] += scores[predecessors[node_id] as usize];
        }

        if max_score < scores[node_id] {
            max_score = scores[node_id];
            max_score_id = node_id as u32;
        }
    }

    return max_score_id as usize;
}

fn po_graph_traverse_heaviest_bundle(
    graph: &mut POGraph,
) {
    let num_nodes = graph.nodes.len();

    let mut predecessors: Vec<i32> = vec![-1; num_nodes];
    let mut scores: Vec<i64> = vec![-1; num_nodes];

    let mut max_score_id = 0;
    for node_id in &graph.rank_to_node_id {
        let node = &graph.nodes[*node_id];
        for edge in &node.in_edges {
            if scores[*node_id] < edge.total_weight ||
                (scores[*node_id] == edge.total_weight &&
                    scores[predecessors[*node_id] as usize] <= scores[edge.begin_node_id]
                ) {
                scores[*node_id] = edge.total_weight;
                predecessors[*node_id] = edge.begin_node_id as i32;
            }
        }

        if predecessors[*node_id] != -1 {
            scores[*node_id] += scores[predecessors[*node_id] as usize];
        }

        if scores[max_score_id] < scores[*node_id] {
            max_score_id = node_id.clone();
        }
    }

    if graph.nodes[max_score_id].out_edges.len() != 0 {
        let mut node_id_to_rank = vec![0; graph.nodes.len()];
        for i in 0..graph.nodes.len() {
            node_id_to_rank[graph.rank_to_node_id[i]] = i;
        }

        loop {
            max_score_id = po_graph_branch_completion(
                &graph, &mut scores, &mut predecessors,
                &node_id_to_rank[max_score_id],
            );

            if graph.nodes[max_score_id].out_edges.len() == 0 {
                break;
            }
        };
    }

    // traceback
    graph.consensus.clear();

    while predecessors[max_score_id] != -1 {
        graph.consensus.push(max_score_id);
        max_score_id = predecessors[max_score_id] as usize;
    }
    graph.consensus.push(max_score_id);

    graph.consensus.reverse();
}

