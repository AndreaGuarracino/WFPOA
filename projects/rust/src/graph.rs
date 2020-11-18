pub(crate) type Alignment = Vec<(Option<usize>, Option<usize>)>;

// todo I want to avoid cloning edges (I need pointer to manage two owner)
#[derive(Clone)]
pub struct POEdge {
    begin_node_id: usize,
    end_node_id: usize,

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
    character: u8,

    in_edges: Vec<POEdge>,
    out_edges: Vec<POEdge>,

    aligned_nodes_ids: Vec<usize>,
}

impl PONode {
    pub fn successor(&self, label: &u32) -> Option<usize> {
        for edge in self.out_edges.iter() {
            for current_label in edge.sequence_labels.iter() {
                if current_label == label {
                    return Some(edge.end_node_id);
                }
            }
        }

        return None;
    }
}

pub struct POGraph {
    nodes: Vec<PONode>,

    rank_to_node_id: Vec<usize>,

    sequences_begin_nodes_ids: Vec<usize>,

    consensus: Vec<usize>,
}

impl std::fmt::Display for POGraph {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let mut graph_to_string = format!("num_sequences: {}\n", self.sequences_begin_nodes_ids.len());
        graph_to_string.push_str(&format!("num_nodes: {}\n", self.nodes.len()));

        for node in &self.nodes {
            graph_to_string.push_str(&format!("\t{}: {}\n", node.id, node.character as char));

            for edge in &node.in_edges {
                graph_to_string.push_str(&format!("\t\t{} <--- {}\n", edge.end_node_id, edge.begin_node_id));
                for label in &edge.sequence_labels {
                    graph_to_string.push_str(&format!("\t\t\tsequence_label: {}\n", label));
                }
            }

            for edge in &node.out_edges {
                graph_to_string.push_str(&format!("\t\t{} ---> {}\n", edge.begin_node_id, edge.end_node_id));
                for label in &edge.sequence_labels {
                    graph_to_string.push_str(&format!("\t\t\tsequence_label: {}\n", label));
                }
            }
        }
        graph_to_string.push_str(&format!("Rank\tNodeId\n"));
        for i_node in self.rank_to_node_id.iter().enumerate() {
            graph_to_string.push_str(&format!("\t{}\t{}\n", i_node.0, i_node.1));
        }

        write!(f, "{}", graph_to_string)
    }
}

impl POGraph {
    pub const fn new() -> POGraph {
        return POGraph {
            nodes: Vec::new(),
            rank_to_node_id: Vec::new(),
            sequences_begin_nodes_ids: Vec::new(),
            consensus: Vec::new(),
        };
    }

    pub fn add_node(
        &mut self,
        character: u8,
    ) -> usize {
        let last_id = self.nodes.len();

        self.nodes.push(PONode {
            id: last_id,
            character,
            in_edges: vec![],
            out_edges: vec![],
            aligned_nodes_ids: vec![],
        });

        return last_id;
    }

    pub fn add_edge(
        &mut self,
        begin_node_id: usize,
        end_node_id: usize,
        weight: u32,
    ) {
        for edge in self.nodes[begin_node_id].out_edges.iter_mut() {
            if edge.end_node_id == end_node_id {
                edge.add_sequence(self.sequences_begin_nodes_ids.len() as u32, weight);

                //+++++++++++++++++++++++++++++++++++
                // todo I need a way to manage that without the shared_ptr (not available in Rust!)
                //  with shared pointer I would not need to update both the memory locations
                for edge in self.nodes[end_node_id].in_edges.iter_mut() {
                    if edge.begin_node_id == begin_node_id {
                        edge.add_sequence(self.sequences_begin_nodes_ids.len() as u32, weight);
                        return;
                    }
                }
                panic!("po_graph_add_edge: edge that was supposed to be present not found!");
                //+++++++++++++++++++++++++++++++++++
            }
        }

        let new_edge: POEdge = POEdge {
            begin_node_id,
            end_node_id,
            sequence_labels: vec![self.sequences_begin_nodes_ids.len() as u32],
            total_weight: weight as i64,
        };

        // todo manage two owners, avoiding duplicating stuff
        self.nodes[begin_node_id].out_edges.push(new_edge.clone());
        self.nodes[end_node_id].in_edges.push(new_edge.clone());
    }

    pub fn add_sequence(
        &mut self,
        sequence: &[u8],
        weights: &[u32],
        begin: usize,
        end: usize) -> Option<usize> {
        if begin == end {
            return None;
        }

        let first_node_id = self.add_node(sequence[begin]);
        for i in (begin + 1)..end {
            let node_id = self.add_node(sequence[i]);

            // both nodes contribute to edge weight
            self.add_edge(node_id - 1, node_id, weights[i - 1] + weights[i]);
        }

        return Some(first_node_id);
    }

    pub fn add_alignment(
        &mut self,
        alignment: &Alignment,
        sequence: &[u8],
        weights: &[u32],
    ) {
        //#ifdef CAUTIOUS_MODE
        if sequence.len() == 0 {
            return;
        }

        if sequence.len() != weights.len() {
            panic!("[wfpoa::po_graph_add_alignment] error: sequence and weights are of unequal size!");
        }
        //#endif

        let mut begin_node_id;
        if alignment.is_empty() { //  no alignment
            begin_node_id = self.add_sequence(sequence, weights, 0, sequence.len());
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

            let tmp = self.nodes.len();
            begin_node_id = self.add_sequence(sequence, weights, 0, valid_seq_ids[0]);
            let mut head_node_id = if tmp == self.nodes.len() { None } else { Some(self.nodes.len() - 1) };

            let tail_node_id = self.add_sequence(
                sequence, weights,
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
                        if self.nodes[*node_id].character == letter {
                            new_node_id = Some(*node_id);
                        } else {
                            let mut aligned_to_node_id: Option<usize> = None;
                            for aligned_node_id in self.nodes[*node_id].aligned_nodes_ids.iter() {
                                if self.nodes[*aligned_node_id].character == letter {
                                    aligned_to_node_id = Some(*aligned_node_id);
                                    break;
                                }
                            }

                            if aligned_to_node_id == None {
                                new_node_id = Some(self.add_node(letter));

                                // todo: I don't like to copy everything to avoid borrowing problem in the for
                                let aligned_nodes_ids = self.nodes[*node_id].aligned_nodes_ids.to_vec();

                                for aligned_node_id in aligned_nodes_ids.iter() {
                                    self.nodes[new_node_id.unwrap()].aligned_nodes_ids.push(*aligned_node_id);
                                    self.nodes[*aligned_node_id].aligned_nodes_ids.push(new_node_id.unwrap());
                                }

                                self.nodes[new_node_id.unwrap()].aligned_nodes_ids.push(*node_id);
                                self.nodes[*node_id].aligned_nodes_ids.push(new_node_id.unwrap());
                            } else {
                                new_node_id = aligned_to_node_id;
                            }
                        }
                    } else {
                        new_node_id = Some(self.add_node(letter));
                    }

                    if begin_node_id == None {
                        begin_node_id = new_node_id;
                    }

                    if let Some(head_node_id) = head_node_id {
                        // both nodes contribute to edge weight
                        self.add_edge(
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
                self.add_edge(
                    head_node_id.unwrap(), tail_node_id,
                    prev_weight + weights[*valid_seq_ids.last().unwrap() + 1],
                );
            }
        }

        self.sequences_begin_nodes_ids.push(begin_node_id.unwrap());

        self.topological_sort();
    }

    fn topological_sort(
        &mut self
    ) {
        self.rank_to_node_id.clear();
        self.rank_to_node_id = Vec::with_capacity(self.nodes.len());

        // 0 - unmarked, 1 - temporarily marked, 2 - permanently marked
        let mut node_marks: Vec<u8> = vec![0; self.nodes.len()];
        let mut do_not_check_aligned_nodes: Vec<bool> = vec![false; self.nodes.len()];
        let mut nodes_to_visit: Vec<usize> = Vec::new();

        for i in 0..self.nodes.len() {
            if node_marks[i] != 0 {
                continue;
            }

            nodes_to_visit.push(i);
            while !nodes_to_visit.is_empty() {
                let node_id = nodes_to_visit.last().unwrap().clone();

                let mut valid = true;

                if node_marks[node_id] != 2 {
                    for edge in &self.nodes[node_id].in_edges {
                        if node_marks[edge.begin_node_id] != 2 {
                            nodes_to_visit.push(edge.begin_node_id);
                            valid = false;
                        }
                    }

                    if !do_not_check_aligned_nodes[node_id] {
                        for aligned_node_id in &self.nodes[node_id].aligned_nodes_ids {
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
                            self.rank_to_node_id.push(node_id);
                            self.rank_to_node_id.extend(self.nodes[node_id].aligned_nodes_ids.to_vec());
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
        assert!(self.is_topologically_sorted());
        //#endif
    }

    fn is_topologically_sorted(
        &self
    ) -> bool {
        //#ifdef CAUTIOUS_MODE
        assert_eq!(self.nodes.len(), self.rank_to_node_id.len());
        //#endif

        let mut visited_nodes = vec![false; self.nodes.len()];

        for i in 0..self.nodes.len() {
            let node_id = &self.rank_to_node_id[i];

            for edge in &self.nodes[*node_id].in_edges {
                if !visited_nodes[edge.begin_node_id] {
                    return false;
                }
            }

            visited_nodes[*node_id] = true;
        }

        return true;
    }

    fn initialize_multiple_sequence_alignment(
        &self,
    ) -> (usize, Vec<usize>) {
        let mut node_id_to_msa_rank = vec![0; self.nodes.len()];

        let mut msa_id: usize = 0;
        let mut i = 0;
        loop {
            let node_id = self.rank_to_node_id[i];

            node_id_to_msa_rank[node_id] = msa_id.clone();

            for _ in 0..self.nodes[node_id].aligned_nodes_ids.len() {
                i += 1;
                node_id_to_msa_rank[self.rank_to_node_id[i]] = msa_id.clone();
            }

            msa_id += 1;

            i += 1;
            if i >= self.nodes.len() {
                break;
            }
        }

        return (msa_id, node_id_to_msa_rank);
    }

    pub fn generate_multiple_sequence_alignment(
        &mut self,
        include_consensus: &bool,
    ) -> (usize, Vec<Vec<u8>>) {
        let (msa_len, node_id_to_msa_rank) = self.initialize_multiple_sequence_alignment();

        let num_sequences = self.sequences_begin_nodes_ids.len() + if *include_consensus { 1 } else { 0 };

        let mut msa_seq = vec![vec![b'-'; msa_len]; num_sequences];
        // extract sequences from graph and create msa strings (add indels(-) where necessary)

        for i in 0..self.sequences_begin_nodes_ids.len() {
            let mut node_id = self.sequences_begin_nodes_ids[i];

            loop {
                msa_seq[i][node_id_to_msa_rank[node_id]] = self.nodes[node_id].character.clone();

                match self.nodes[node_id].successor(&(i as u32)) {
                    Some(new_node_id) => node_id = new_node_id,
                    None => break
                }
            }
        }

        if *include_consensus {
            // do the same for consensus sequence
            self.traverse_heaviest_bundle();

            for node_id in &self.consensus {
                msa_seq[num_sequences - 1][node_id_to_msa_rank[*node_id]] = self.nodes[*node_id].character.clone();
            }
        }

        return (msa_len, msa_seq);
    }

    fn branch_completion(
        &self,
        scores: &mut Vec<i64>,
        predecessors: &mut Vec<i32>,
        rank: &usize,
    ) -> usize {
        let node_id = self.rank_to_node_id[*rank].clone();

        for out_edge in &self.nodes[node_id].out_edges {
            for in_edge in &self.nodes[out_edge.end_node_id].in_edges {
                if in_edge.begin_node_id != node_id {
                    scores[out_edge.begin_node_id] = -1;
                }
            }
        }

        let mut max_score: i64 = 0;
        let mut max_score_id: u32 = 0;
        for i in (rank + 1)..self.nodes.len() {
            let node_id = self.rank_to_node_id[i].clone();
            let node = &self.nodes[node_id];

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

    fn traverse_heaviest_bundle(
        &mut self,
    ) {
        let num_nodes = self.nodes.len();

        let mut predecessors: Vec<i32> = vec![-1; num_nodes];
        let mut scores: Vec<i64> = vec![-1; num_nodes];

        let mut max_score_id = 0;
        for node_id in &self.rank_to_node_id {
            let node = &self.nodes[*node_id];
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

        if self.nodes[max_score_id].out_edges.len() != 0 {
            let mut node_id_to_rank = vec![0; self.nodes.len()];
            for i in 0..self.nodes.len() {
                node_id_to_rank[self.rank_to_node_id[i]] = i;
            }

            loop {
                max_score_id = self.branch_completion(
                    &mut scores, &mut predecessors,
                    &node_id_to_rank[max_score_id],
                );

                if self.nodes[max_score_id].out_edges.len() == 0 {
                    break;
                }
            };
        }

        // traceback
        self.consensus.clear();

        while predecessors[max_score_id] != -1 {
            self.consensus.push(max_score_id);
            max_score_id = predecessors[max_score_id] as usize;
        }
        self.consensus.push(max_score_id);

        self.consensus.reverse();
    }
}
