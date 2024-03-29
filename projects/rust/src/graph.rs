use std::cell::RefCell;
use std::rc::Rc;

use fxhash::FxHashMap;

use std::fs::File;
use std::io::{LineWriter, Write};

const START_NODE_ID: usize = 0;
//const END_NODE_ID: usize = 1;

pub type Alignment = Vec<(Option<usize>, Option<usize>)>;

pub struct POEdge {
    pub begin_node_id: usize,
    pub end_node_id: usize,

    sequence_labels: Vec<u32>,

    total_weight: i64,
}

impl POEdge {
    pub fn add_sequence(&mut self, label: u32, weight: u32) {
        self.sequence_labels.push(label);
        self.total_weight += weight as i64;
    }
}

pub struct PONode {
    id: usize,
    pub character: u8,

    pub in_edges: Vec<Rc<RefCell<POEdge>>>,
    pub out_edges: Vec<Rc<RefCell<POEdge>>>,

    aligned_nodes_ids: Vec<usize>,
}

impl PONode {
    pub fn successor(&self, label: &u32) -> Option<usize> {
        for edge_ in self.out_edges.iter() {
            let edge = edge_.borrow();

            for current_label in edge.sequence_labels.iter() {
                if current_label == label {
                    return Some(edge.end_node_id);
                }
            }
        }

        None
    }
}

pub struct POGraph {
    pub nodes: Vec<PONode>,

    pub rank_to_node_id: Vec<usize>,
    //pub node_id_to_rank: Vec<usize>,

    // ToDo: if a START_NODE will be used, replace this vector with the START_NODE's outgoing edges
    sequences_begin_nodes_ids: Vec<usize>,

    consensus: Vec<usize>,
}

impl std::fmt::Display for POGraph {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let mut graph_to_string =
            format!("num_sequences: {}\n", self.sequences_begin_nodes_ids.len());
        graph_to_string.push_str(&format!("num_nodes: {}\n", self.nodes.len()));

        let mut node_id_to_rank = vec![0; self.nodes.len()];
        for rank in 0..self.nodes.len() {
            node_id_to_rank[self.rank_to_node_id[rank]] = rank;
        }

        for node_id in &self.rank_to_node_id {
            let node = &self.nodes[*node_id];

            graph_to_string.push_str(&format!("\t{}: {}\n", node.id, node.character as char));

            for edge_ in &node.in_edges {
                let edge = edge_.borrow();

                graph_to_string.push_str(&format!(
                    "\t\t{} ({}) <--- {} ({}) - W: {}\n",
                    node_id_to_rank[edge.end_node_id],
                    self.nodes[edge.end_node_id].character as char,
                    node_id_to_rank[edge.begin_node_id],
                    self.nodes[edge.begin_node_id].character as char,
                    edge.total_weight
                ));
                for label in &edge.sequence_labels {
                    graph_to_string.push_str(&format!("\t\t\tsequence_label: {}\n", label));
                }
            }

            for edge_ in &node.out_edges {
                let edge = edge_.borrow();

                graph_to_string.push_str(&format!(
                    "\t\t{} ({}) ---> {} ({}) - W: {}\n",
                    node_id_to_rank[edge.begin_node_id],
                    self.nodes[edge.begin_node_id].character as char,
                    node_id_to_rank[edge.end_node_id],
                    self.nodes[edge.end_node_id].character as char,
                    edge.total_weight
                ));
                for label in &edge.sequence_labels {
                    graph_to_string.push_str(&format!("\t\t\tsequence_label: {}\n", label));
                }
            }
        }
        graph_to_string.push_str("Rank\tNodeId\n");
        for i_node in self.rank_to_node_id.iter().enumerate() {
            graph_to_string.push_str(&format!(
                "\t{}\t{} ({})\n",
                i_node.0, i_node.1, self.nodes[*i_node.1].character as char
            ));
        }

        write!(f, "{}", graph_to_string)
    }
}

impl POGraph {
    pub fn new(num_initial_sequences: usize, num_initial_nodes: usize) -> POGraph {
        //graph.add_node(b'S');
        //graph.add_node(b'E');

        POGraph {
            nodes: Vec::with_capacity(num_initial_nodes /* + 2*/),
            rank_to_node_id: Vec::with_capacity(num_initial_nodes /* + 2*/),
            sequences_begin_nodes_ids: Vec::with_capacity(num_initial_sequences),
            consensus: Vec::with_capacity(num_initial_nodes),
        }
    }

    pub fn graph_sequence(&self) -> Vec<u8> {
        return self
            .rank_to_node_id
            .iter()
            .map(|node_id| self.nodes[*node_id].character)
            .collect();
    }

    pub fn add_node(&mut self, character: u8) -> usize {
        let last_id = self.nodes.len();

        self.nodes.push(PONode {
            id: last_id,
            character,
            in_edges: Vec::new(),
            out_edges: Vec::new(),
            aligned_nodes_ids: Vec::new(),
        });

        last_id
    }

    pub fn add_edge(&mut self, begin_node_id: usize, end_node_id: usize, weight: u32) {
        for edge_ in self.nodes[begin_node_id].out_edges.iter_mut() {
            let mut edge = edge_.borrow_mut();

            if edge.end_node_id == end_node_id {
                edge.add_sequence(self.sequences_begin_nodes_ids.len() as u32, weight);
                return;
            }
        }

        let new_edge = Rc::new(RefCell::new(POEdge {
            begin_node_id,
            end_node_id,
            sequence_labels: vec![self.sequences_begin_nodes_ids.len() as u32],
            total_weight: weight as i64,
        }));

        self.nodes[begin_node_id].out_edges.push(new_edge.clone());
        self.nodes[end_node_id].in_edges.push(new_edge);
    }

    pub fn add_sequence(
        &mut self,
        sequence: &[u8],
        weights: &[u32],
        begin: usize,
        end: usize,
    ) -> (Option<usize>, Option<usize>) {
        if begin == end {
            return (None, None);
        }

        let first_node_id = self.add_node(sequence[begin]);
        let mut prev_node_id = first_node_id;
        let mut curr_node_id = 0;
        for i in (begin + 1)..end {
            curr_node_id = self.add_node(sequence[i]);

            // both nodes contribute to edge weight
            self.add_edge(prev_node_id, curr_node_id, weights[i - 1] + weights[i]);

            prev_node_id = curr_node_id;
        }

        (Some(first_node_id), Some(curr_node_id))
    }

    pub fn add_alignment(&mut self, alignment: &Alignment, sequence: &[u8], weights: &[u32]) {
        //#ifdef CAUTIOUS_MODE
        if sequence.is_empty() {
            return;
        }
        //#endif

        debug_assert_eq!(
            sequence.len(),
            weights.len(),
            "[wfpoa::POGraph::add_alignment] error: sequence and weights are of unequal size"
        );

        let mut begin_node_id;
        if alignment.is_empty() {
            //  no alignment
            let begin_end_node_ids = self.add_sequence(sequence, weights, 0, sequence.len());
            begin_node_id = begin_end_node_ids.0;

            //self.add_edge(begin_end_node_ids.1.unwrap(), END_NODE_ID, *weights.last().unwrap());
        } else {
            let mut valid_seq_ids: Vec<usize> = Vec::with_capacity(alignment.len());
            for align in alignment.iter() {
                if let Some(seq_id) = align.1 {
                    debug_assert!(
                        seq_id < sequence.len(),
                        "[wfpoa::POGraph::add_alignment] error: invalid alignment"
                    );

                    valid_seq_ids.push(seq_id);
                }
            }

            debug_assert!(
                !valid_seq_ids.is_empty(),
                "[wfpoa::POGraph::add_alignment] error: missing sequence in the alignment"
            );

            // Add unaligned bases
            let begin_end_node_ids = self.add_sequence(sequence, weights, 0, valid_seq_ids[0]);
            begin_node_id = begin_end_node_ids.0;
            let mut prev_node_id = begin_end_node_ids.1;

            let last_node_id = self
                .add_sequence(
                    sequence,
                    weights,
                    *valid_seq_ids.last().unwrap() + 1,
                    sequence.len(),
                )
                .0;

            // Add aligned bases
            let mut curr_node_id: Option<usize>;
            for (node_id, seq_id) in alignment.iter() {
                if let Some(seq_id) = seq_id {
                    let letter = sequence[*seq_id];

                    if let Some(node_id) = node_id {
                        if self.nodes[*node_id].character == letter {
                            curr_node_id = Some(*node_id);
                        } else {
                            curr_node_id = None;
                            for aligned_node_id in self.nodes[*node_id].aligned_nodes_ids.iter() {
                                if self.nodes[*aligned_node_id].character == letter {
                                    curr_node_id = Some(*aligned_node_id);
                                    break;
                                }
                            }

                            if curr_node_id == None {
                                curr_node_id = Some(self.add_node(letter));

                                // todo: I don't like to copy everything to avoid borrowing problems in the for
                                let aligned_nodes_ids =
                                    self.nodes[*node_id].aligned_nodes_ids.to_vec();

                                for aligned_node_id in aligned_nodes_ids.iter() {
                                    self.nodes[curr_node_id.unwrap()]
                                        .aligned_nodes_ids
                                        .push(*aligned_node_id);
                                    self.nodes[*aligned_node_id]
                                        .aligned_nodes_ids
                                        .push(curr_node_id.unwrap());
                                }

                                self.nodes[curr_node_id.unwrap()]
                                    .aligned_nodes_ids
                                    .push(*node_id);
                                self.nodes[*node_id]
                                    .aligned_nodes_ids
                                    .push(curr_node_id.unwrap());
                            }
                        }
                    } else {
                        curr_node_id = Some(self.add_node(letter));
                    }

                    if begin_node_id == None {
                        begin_node_id = curr_node_id;
                    }

                    if let Some(prev_node_id) = prev_node_id {
                        // Both nodes contribute to edge weight
                        self.add_edge(
                            prev_node_id,
                            curr_node_id.unwrap(),
                            weights[*seq_id - 1] + weights[*seq_id],
                        );
                    }

                    prev_node_id = curr_node_id;
                }
            }

            if let Some(last_node_id) = last_node_id {
                // Both nodes contribute to edge weight
                self.add_edge(
                    prev_node_id.unwrap(),
                    last_node_id,
                    weights[*valid_seq_ids.last().unwrap()]
                        + weights[*valid_seq_ids.last().unwrap() + 1],
                );

                //prev_node_id = Some(last_node_id);
            }

            //self.add_edge(prev_node_id.unwrap(), END_NODE_ID, *weights.last().unwrap());
        }

        //self.add_edge(START_NODE_ID, begin_node_id.unwrap(), 1 + weights[0]);

        self.sequences_begin_nodes_ids.push(begin_node_id.unwrap());

        self._topological_sort_spoa();
    }

    // Kahn’s algorithm: it needs the list of "start nodes" which have no incoming edges
    fn _topological_sort_kahn(&mut self) {
        self.rank_to_node_id.resize(self.nodes.len(), 0);

        let mut rank = 0;

        // O(V)
        let mut in_degree = vec![0; self.nodes.len()];
        for node in &self.nodes {
            in_degree[node.id] = node.in_edges.len();
        }

        // Initialize count of visited vertices
        let mut num_visited_vertices: usize = 0;

        //let mut node_ids_queue = vec![START_NODE_ID; 1];
        let mut node_ids_queue: Vec<usize> = self
            .sequences_begin_nodes_ids
            .iter()
            .cloned()
            .filter(|node_id| self.nodes[*node_id].in_edges.is_empty())
            .collect();
        node_ids_queue.sort();
        node_ids_queue.dedup();

        while !node_ids_queue.is_empty() {
            let node_id = node_ids_queue.pop().unwrap();

            self.rank_to_node_id[rank] = node_id;
            rank += 1;

            for edge_ in &self.nodes[node_id].out_edges {
                let end_node_id = edge_.borrow().end_node_id;
                in_degree[end_node_id] -= 1;

                if in_degree[end_node_id] == 0 {
                    node_ids_queue.push(end_node_id);
                }
            }

            num_visited_vertices += 1;
        }

        // Check if there was a cycle
        debug_assert_eq!(
            num_visited_vertices,
            self.nodes.len(),
            "[wfpoa::POGraph::topological_sort] error: graph is not a DAG"
        );
    }

    // It needs a single "start node" which have no incoming edges
    fn _topological_sort_abpoa(&mut self) {
        self.rank_to_node_id.resize(self.nodes.len(), 0);
        //self.node_id_to_rank.resize(self.nodes.len(), 0);

        let mut rank = 0;

        // O(V)
        let mut in_degree = vec![0; self.nodes.len()];
        for node in &self.nodes {
            in_degree[node.id] = node.in_edges.len();
        }

        let mut node_ids_queue = vec![START_NODE_ID; 1];

        // Breadth-First-Search
        while !node_ids_queue.is_empty() {
            let node_id = node_ids_queue.pop().unwrap();

            self.rank_to_node_id[rank] = node_id;
            //self.node_id_to_rank[node_id] = rank;
            rank += 1;

            for edge_ in &self.nodes[node_id].out_edges {
                let end_node_id = edge_.borrow().end_node_id;
                in_degree[end_node_id] -= 1;

                if in_degree[end_node_id] == 0 {
                    let mut add_nodes = true;
                    for aligned_node_id in &self.nodes[end_node_id].aligned_nodes_ids {
                        if in_degree[*aligned_node_id] != 0 {
                            add_nodes = false;
                            break;
                        }
                    }
                    if add_nodes {
                        node_ids_queue.push(end_node_id);
                        node_ids_queue.extend(self.nodes[end_node_id].aligned_nodes_ids.iter());
                    }
                }
            }
        }
    }

    fn _topological_sort_spoa(&mut self) {
        self.rank_to_node_id.resize(self.nodes.len(), 0);

        let mut rank = 0;

        // 0) unmarked, 1) temporarily marked, 2) permanently marked
        let mut node_marks: Vec<u8> = vec![0; self.nodes.len()];
        let mut check_aligned_nodes: Vec<bool> = vec![true; self.nodes.len()];

        let mut node_ids_to_visit: Vec<usize> = Vec::new();

        for node in &self.nodes {
            if node_marks[node.id] != 0 {
                continue;
            }

            node_ids_to_visit.push(node.id);
            while !node_ids_to_visit.is_empty() {
                let node_id = *node_ids_to_visit.last().unwrap();

                let mut is_valid = true;

                if node_marks[node_id] != 2 {
                    for edge_ in &self.nodes[node_id].in_edges {
                        let edge = edge_.borrow();

                        if node_marks[edge.begin_node_id] != 2 {
                            node_ids_to_visit.push(edge.begin_node_id);
                            is_valid = false;
                        }
                    }

                    if check_aligned_nodes[node_id] {
                        for aligned_node_id in &self.nodes[node_id].aligned_nodes_ids {
                            if node_marks[*aligned_node_id] != 2 {
                                node_ids_to_visit.push(*aligned_node_id);
                                check_aligned_nodes[*aligned_node_id] = false;
                                is_valid = false;
                            }
                        }
                    }

                    debug_assert!(
                        is_valid || node_marks[node_id] != 1,
                        "[wfpoa::POGraph::topological_sort] error: graph is not a DAG"
                    );

                    if is_valid {
                        node_marks[node_id] = 2;

                        if check_aligned_nodes[node_id] {
                            self.rank_to_node_id[rank] = node_id;
                            rank += 1;

                            for aligned_node_id in &self.nodes[node_id].aligned_nodes_ids {
                                self.rank_to_node_id[rank] = *aligned_node_id;
                                rank += 1;
                            }
                        }
                    } else {
                        node_marks[node_id] = 1;
                    }
                }

                if is_valid {
                    node_ids_to_visit.pop();
                }
            }
        }

        debug_assert!(
            self._is_topologically_sorted(),
            "[wfpoa::POGraph::topological_sort] error: graph is not topologically sorted"
        );
    }

    fn _is_topologically_sorted(&self) -> bool {
        debug_assert_eq!(self.nodes.len(), self.rank_to_node_id.len());

        let mut visited_nodes = vec![false; self.nodes.len()];

        for node_id in &self.rank_to_node_id {
            for edge_ in &self.nodes[*node_id].in_edges {
                if !visited_nodes[edge_.borrow().begin_node_id] {
                    return false;
                }
            }

            visited_nodes[*node_id] = true;
        }

        true
    }

    // It assumes consecutive ranks for the aligned nodes
    fn _initialize_multiple_sequence_alignment_spoa(&self) -> (usize, Vec<usize>) {
        let num_nodes_minus_1 = self.nodes.len() - 1;

        let mut node_id_to_msa_column = vec![0; self.nodes.len()];

        let mut i = 0;
        let mut column = 0;
        loop {
            let node_id = self.rank_to_node_id[i];

            node_id_to_msa_column[node_id] = column;

            for aligned_node_id in &self.nodes[node_id].aligned_nodes_ids {
                node_id_to_msa_column[*aligned_node_id] = column;
                i += 1;
            }

            if i >= num_nodes_minus_1 {
                break;
            }

            i += 1;
            column += 1;
        }

        (column + 1, node_id_to_msa_column)
    }

    // It needs a single "start node" which have no incoming edges
    fn _initialize_multiple_sequence_alignment_abpoa(&self) -> (usize, Vec<usize>) {
        // O(V)
        let mut in_degree = vec![0; self.nodes.len()];
        for node in &self.nodes {
            in_degree[node.id] = node.in_edges.len();
        }

        let mut column = 0;
        let mut node_id_to_msa_column = vec![usize::MAX; self.nodes.len()];

        let mut node_ids_queue = vec![START_NODE_ID; 1];

        // Breadth-First-Search
        while !node_ids_queue.is_empty() {
            let node_id = node_ids_queue.pop().unwrap();

            if node_id_to_msa_column[node_id] == usize::MAX {
                node_id_to_msa_column[node_id] = column;

                for aligned_node_id in &self.nodes[node_id].aligned_nodes_ids {
                    node_id_to_msa_column[*aligned_node_id] = column;
                }
                column += 1;
            }

            for edge_ in &self.nodes[node_id].out_edges {
                let end_node_id = edge_.borrow().end_node_id;
                in_degree[end_node_id] -= 1;

                if in_degree[end_node_id] == 0 {
                    let mut add_nodes = true;
                    for aligned_node_id in &self.nodes[end_node_id].aligned_nodes_ids {
                        if in_degree[*aligned_node_id] != 0 {
                            add_nodes = false;
                            break;
                        }
                    }
                    if add_nodes {
                        node_ids_queue.push(end_node_id);
                        node_ids_queue.extend(self.nodes[end_node_id].aligned_nodes_ids.iter());
                    }
                }
            }
        }

        (column, node_id_to_msa_column)
    }

    pub fn generate_multiple_sequence_alignment(
        &mut self,
        include_consensus: &bool,
    ) -> (usize, Vec<Vec<u8>>) {
        let (msa_len, node_id_to_msa_column) = self._initialize_multiple_sequence_alignment_spoa();

        let num_sequences =
            self.sequences_begin_nodes_ids.len() + if *include_consensus { 1 } else { 0 };

        let mut msa_seq = vec![vec![b'-'; msa_len]; num_sequences];

        // Extract sequences from graph and create MSA strings
        for label in 0..self.sequences_begin_nodes_ids.len() {
            //msa_seq[i][0] = b'S';

            let mut node_id = self.sequences_begin_nodes_ids[label];

            loop {
                msa_seq[label][node_id_to_msa_column[node_id]] = self.nodes[node_id].character;

                node_id = match self.nodes[node_id].successor(&(label as u32)) {
                    Some(new_node_id) => new_node_id,
                    None => break,
                }
            }
        }

        if *include_consensus {
            // do the same for consensus sequence
            self.traverse_heaviest_bundle();

            for node_id in &self.consensus {
                msa_seq[num_sequences - 1][node_id_to_msa_column[*node_id]] =
                    self.nodes[*node_id].character;
            }
        }

        (msa_len, msa_seq)
    }

    fn branch_completion(
        &self,
        scores: &mut Vec<i64>,
        predecessors: &mut Vec<usize>,
        rank: &usize,
    ) -> usize {
        let node_id = self.rank_to_node_id[*rank];

        for out_edge_ in &self.nodes[node_id].out_edges {
            let out_edge = out_edge_.borrow();

            for in_edge_ in &self.nodes[out_edge.end_node_id].in_edges {
                if in_edge_.borrow().begin_node_id != node_id {
                    scores[out_edge.begin_node_id] = -1;
                }
            }
        }

        let mut max_score_id = *rank;
        for i in (rank + 1)..self.nodes.len() {
            let node_id = self.rank_to_node_id[i];
            let node = &self.nodes[node_id];

            scores[node_id] = -1;
            predecessors[node_id] = usize::MAX;

            for edge_ in &node.in_edges {
                let edge = edge_.borrow();

                if scores[edge.begin_node_id] == -1 {
                    continue;
                }

                if scores[node_id] < edge.total_weight
                    || (scores[node_id] == edge.total_weight
                        && scores[predecessors[node_id]] <= scores[edge.begin_node_id])
                {
                    scores[node_id] = edge.total_weight;
                    predecessors[node_id] = edge.begin_node_id;
                }
            }

            if predecessors[node_id] != usize::MAX {
                scores[node_id] += scores[predecessors[node_id]];
            }

            if scores[max_score_id] < scores[node_id] {
                max_score_id = node_id;
            }
        }

        max_score_id
    }

    fn traverse_heaviest_bundle(&mut self) {
        debug_assert!(!self.rank_to_node_id.is_empty());

        let mut predecessors: Vec<usize> = vec![usize::MAX; self.nodes.len()];
        let mut scores: Vec<i64> = vec![-1; self.nodes.len()];

        let mut max_score_id = 0;
        for node_id in &self.rank_to_node_id {
            let node = &self.nodes[*node_id];
            for edge_ in &node.in_edges {
                let edge = edge_.borrow();

                if scores[*node_id] < edge.total_weight
                    || (scores[*node_id] == edge.total_weight
                        && scores[predecessors[*node_id]] <= scores[edge.begin_node_id])
                {
                    scores[*node_id] = edge.total_weight;
                    predecessors[*node_id] = edge.begin_node_id;
                }
            }

            if predecessors[*node_id] != usize::MAX {
                scores[*node_id] += scores[predecessors[*node_id]];
            }

            if scores[max_score_id] < scores[*node_id] {
                max_score_id = *node_id;
            }
        }

        if !self.nodes[max_score_id].out_edges.is_empty() {
            let mut node_id_to_rank = vec![0; self.nodes.len()];
            for i in 0..self.nodes.len() {
                node_id_to_rank[self.rank_to_node_id[i]] = i;
            }

            loop {
                max_score_id = self.branch_completion(
                    &mut scores,
                    &mut predecessors,
                    &node_id_to_rank[max_score_id],
                );

                if self.nodes[max_score_id].out_edges.is_empty() {
                    break;
                }
            }
        }

        // Traceback
        self.consensus.clear();

        while predecessors[max_score_id] != usize::MAX {
            self.consensus.push(max_score_id);
            max_score_id = predecessors[max_score_id];
        }
        self.consensus.push(max_score_id);

        self.consensus.reverse();
    }

    pub fn graph_2_dot(&self) -> std::io::Result<()> {
        /*
        // all settings
        // float dpi_size = 3000, graph_width = 100, graph_height = 6;
         */

        let font_size = 22;

        let mut node_color = FxHashMap::with_capacity_and_hasher(7, Default::default());
        node_color.insert(b'A', "lightskyblue");
        node_color.insert(b'C', "salmon");
        node_color.insert(b'G', "lightgoldenrod");
        node_color.insert(b'T', "limegreen");
        node_color.insert(b'N', "gray");
        node_color.insert(b'S', "thistle");
        node_color.insert(b'E', "thistle");

        let mut node_label =
            FxHashMap::with_capacity_and_hasher(self.nodes.len(), Default::default());

        let node_width = 1.2;
        let rankdir = "LR"; //TB or LR
        let node_style = "filled";
        let node_fixedsize = "true";
        let node_shape = "circle";

        let show_aligned_mismatch = true;

        let mut graph_to_dot = format!("// WFPOA graph dot file\n//{} nodes.\n", self.nodes.len());
        graph_to_dot.push_str(&format!(
            "digraph WFPOA_graph {{\n\tgraph [rankdir=\"{}\"];\n\tnode [width={}, style={}, fixedsize={}, shape={}];\n", rankdir, node_width, node_style, node_fixedsize, node_shape
        ));

        // Prepare node labels and write node color and fontsize
        for (rank, node_id) in self.rank_to_node_id.iter().enumerate() {
            let node = &self.nodes[*node_id];

            node_label.insert(
                node_id,
                format!("{} ({})\nr: {}", node.character as char, node_id, rank),
            );

            graph_to_dot.push_str(&format!(
                "\"{}\" [color={}, fontsize={}]\n",
                node_label[node_id], node_color[&node.character], font_size
            ));
        }

        let mut node_id_to_rank = Vec::new();
        if show_aligned_mismatch {
            node_id_to_rank = vec![0; self.nodes.len()];
            for i in 0..self.nodes.len() {
                node_id_to_rank[self.rank_to_node_id[i]] = i;
            }
        }

        let mut x_index = 0;
        for (rank, node_id) in self.rank_to_node_id.iter().enumerate() {
            let node = &self.nodes[*node_id];

            // Out edges
            for edge_ in &node.out_edges {
                let edge = edge_.borrow();
                let out_weight = edge.sequence_labels.len(); //abg->node[id].out_weight[j]+1

                graph_to_dot.push_str(&format!(
                    "\t\"{}\" -> \"{}\" [label=\"{}\", penwidth={}]\n",
                    node_label[node_id],
                    node_label[&edge.end_node_id],
                    out_weight,
                    out_weight + 1
                ));
            }

            if !node.aligned_nodes_ids.is_empty() {
                graph_to_dot.push_str(&format!("\t{{rank=same; \"{}\" ", node_label[node_id]));
                for aligned_node_id in &node.aligned_nodes_ids {
                    graph_to_dot.push_str(&format!("\"{}\" ", node_label[aligned_node_id]));
                }
                graph_to_dot.push_str("};\n");

                if show_aligned_mismatch && rank > x_index {
                    x_index = rank;

                    // Mismatch dashed line
                    graph_to_dot.push_str(&format!(
                        "\t{{ edge [style=dashed, arrowhead=none]; \"{}\" ",
                        node_label[node_id]
                    ));

                    for aligned_node_id in &node.aligned_nodes_ids {
                        graph_to_dot.push_str(&format!("-> \"{}\" ", node_label[aligned_node_id]));

                        let index = node_id_to_rank[*aligned_node_id];
                        if index > x_index {
                            x_index = index;
                        }
                    }

                    graph_to_dot.push_str("}\n");
                }
            }
        }
        graph_to_dot.push_str("}\n");

        let file = File::create("WFPOA_graph.dot")?;
        let mut file = LineWriter::new(file);
        file.write_all(graph_to_dot.as_bytes())
    }
}
