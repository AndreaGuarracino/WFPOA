use fxhash::FxHashMap; // Faster
//use fnv::FnvHashMap;

macro_rules! max {
    ($x: expr, $y: expr) => {{
        if $x > $y { $x } else { $y }
    }}
}

macro_rules! abs {
    ($x: expr) => {{
        if $x >= 0 { $x } else { -$x }
    }}
}

macro_rules! ewavefront_diagonal {
    ($h: expr, $v: expr) => {{
        $h - $v
    }}
}

macro_rules! ewavefront_offset {
    ($h: expr, $v: expr) => {{
        $h
    }}
}

macro_rules! ewavefront_v {
    ($k: expr, $offset: expr) => {{
        $offset - $k
    }}
}

macro_rules! ewavefront_h {
    ($k: expr, $offset: expr) => {{
        $offset
    }}
}

type EwfOffsetT = i16;

struct EditWavefrontT {
    lo: isize,
    // Effective lowest diagonal (inclusive)
    hi: isize,
    // Effective highest diagonal (inclusive)
    offsets: FxHashMap<isize, EwfOffsetT>, // Offsets
}

struct EditWavefrontsT {
    // Dimensions
    pattern_length: usize,
    text_length: usize,
    max_distance: usize,

    // Waves Offsets
    wavefronts: Vec<EditWavefrontT>,
    wavefronts_allocated: usize,

    // CIGAR
    edit_cigar: Vec<u8>,
    edit_cigar_length: usize,
}

fn edit_wavefronts_backtrace(
    wavefronts: &mut EditWavefrontsT,
    _pattern: &[u8], _text: &[u8],
    target_k: isize, target_distance: usize,
) {
    //print!("edit_wavefronts_backtrace\n");

    // Parameters
    let mut k = target_k;
    let mut distance = target_distance;

    let wavefronts_slice = &mut wavefronts.wavefronts[..=target_distance];

    let mut offset = wavefronts_slice[target_distance].offsets[&k];

    wavefronts.edit_cigar_length = 0;

    while distance > 0 {
        /*print!("\tdistance: {}\n", distance);
        print!("\tk: {}\n", k);*/

        // Fetch
        let wavefront = &wavefronts_slice[distance - 1];

        /*print!("\tdistance - 1: {}\n", distance - 1);
        print!("\t\twavefront->lo: {}\n", wavefront.lo);
        print!("\t\twavefront->hi: {}\n", wavefront.hi);
        if wavefront.lo <= k - 1 && k - 1 <= wavefront.hi {
            print!("\t\toffsets[{}]: {}\n", k - 1, wavefront.offsets[(k - 1 + (-lo)) as usize]);
        }
        print!("\t\toffset[{}]: {}\n", k - 1, offset);
        if wavefront.lo <= k + 1 && k + 1 <= wavefront.hi {
            print!("\t\toffsets[{}]: {}\n", k + 1, wavefront.offsets[(k + 1 + (-lo)) as usize]);
        }*/

        // Traceback operation
        if wavefront.lo <= k + 1 && k + 1 <= wavefront.hi && offset == wavefront.offsets[&(k + 1)] {
            wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'D';
            wavefronts.edit_cigar_length += 1;
            k += 1;
            distance -= 1;
        } else if wavefront.lo <= k - 1 && k - 1 <= wavefront.hi && offset == wavefront.offsets[&(k - 1)] + 1 {
            wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'I';
            wavefronts.edit_cigar_length += 1;
            k -= 1;
            offset -= 1;
            distance -= 1;
        } else if wavefront.lo <= k && k <= wavefront.hi && offset == wavefront.offsets[&k] + 1 {
            wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'X';
            wavefronts.edit_cigar_length += 1;
            distance -= 1;
            offset -= 1;
        } else {
            wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'M';
            wavefronts.edit_cigar_length += 1;
            offset -= 1;
        }

        //print!("\t\t{}\n", wavefronts.edit_cigar.chars().last().unwrap());
    }

    // Account for last offset of matches
    while offset > 0 {
        wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'M';
        wavefronts.edit_cigar_length += 1;
        offset -= 1;
    }
}

fn edit_wavefronts_extend_wavefront(
    wavefront: &mut EditWavefrontT,
    pattern: &[u8], pattern_length: usize,
    text: &[u8], text_length: usize,
    _distance: usize,
) {
    // Extend diagonally each wavefront point
    for (k, offset) in wavefront.offsets.iter_mut() {
        let mut v = ewavefront_v!(*k, *offset as isize) as usize; // offsets[k]-k
        let mut h = ewavefront_h!(*k, *offset as isize) as usize; // offsets[k]

        /*print!("\tedit_wavefronts_extend_wavefront\n");
        print!("\t\tk: {}\n", k);*/
        while v < pattern_length && h < text_length && pattern[v] == text[h] {
            //print!("{}\t{}\t{}\t{}\t{}\tM\t{}\t{}\n", v, h, *offset, _distance, k, pattern[v] as char, text[h] as char);
            /*print!("\t\t\twavefronts[{}]->offsets[{}]: {}\n", _distance, k, offset);
            print!("\t\t\t(v, h) == ({}, {}) ==> ({}, {})\n", v, h, pattern[v] as char, text[h] as char);*/

            *offset += 1;
            v += 1;
            h += 1;
        }

        //print!("{}\t{}\t{}\t{}\t{}\tX\t{}\t{}\n", v, h, *offset, _distance, k, pattern[v] as char, text[h] as char);
        /*print!("\t\t\twavefronts[{}]->offsets[{}]: {}\n", _distance, k, offset);
        print!("\t\t\t(v, h) == ({}, {}) ==> ({}, {})\n", v, h, pattern[v] as char, text[h] as char);*/
    }
    //print!("\t---------------\n");
}

fn edit_wavefronts_compute_wavefront(
    wavefronts: &mut EditWavefrontsT,
    _pattern_length: usize, _text_length: usize,
    distance: usize,
) {
    //print!("\tedit_wavefronts_compute_wavefront\n");

    // Fetch wavefronts
    //let wavefront_next_wavefront = &mut wavefronts.wavefronts[distance - 1..=distance];
    let (xxx, yyy) = wavefronts.wavefronts.split_at_mut(distance);
    let wf_prec = &mut xxx[distance - 1];
    let wf_succ = &mut yyy[0];

    edit_wavefronts_allocate_wavefront(
        wf_succ, wf_prec.lo - 1, wf_prec.hi + 1,
    );
    // Allocate wavefront
    wavefronts.wavefronts_allocated += 1; // Next
    /*print!("\t\tedit_wavefronts->wavefronts_allocated: {}\n", wavefronts.wavefronts_allocated);
    print!("\t---------------\n");*/

    // Fetch offsets
    let wf_prec_offset_lo = wf_prec.offsets[&wf_prec.lo];

    // Loop peeling (k=lo-1); there is only the Upper DP cell (there are no Mid and Lower cells)
    wf_succ.offsets.insert(wf_prec.lo - 1, wf_prec_offset_lo);

    // Loop peeling (k=lo)
    let bottom_upper_del = if (wf_prec.lo + 1) <= wf_prec.hi { wf_prec.offsets[&(wf_prec.lo + 1)] } else { -1 };
    wf_succ.offsets.insert(wf_prec.lo, max!(wf_prec_offset_lo + 1, bottom_upper_del));

    /*print!("\t\tlo - 1, next_wavefront[{}]->next_offsets[{}]: {}\n", distance, lo - 1, wf_succ.offsets[(lo - 1 + (-wf_succ.lo)) as usize]);
    //print!("\t\tlo - 1, wavefront[{}]->offsets[{}]: {}\n", distance - 1, lo - 1, wf_prec.offsets[(lo - 1 + (-wf_prec.lo)) as usize]);
    print!("\t\tlo    , next_wavefront[{}]->next_offsets[{}]: {}\n", distance, lo, wf_succ.offsets[(lo + (-wf_succ.lo)) as usize]);
    print!("\t\tlo    , wavefront[{}]->offsets[{}]: {}\n", distance - 1, lo, wf_prec.offsets[(lo + (-wf_prec.lo)) as usize]);*/

    // Compute next wavefront starting point
    for k in (wf_prec.lo + 1)..wf_prec.hi {
        /*
         *           deletion <-- wf_prec.offsets[k + 1];     // Upper
         *       substitution <-- wf_prec.offsets[k] + 1;     // Mid
         *          insertion <-- wf_prec.offsets[k - 1] + 1; // Lower
         * wf_succ.offsets[k] <-- MAX(substitution, insertion, deletion); // MAX
         */
        let max_ins_sub = max!(wf_prec.offsets[&k], wf_prec.offsets[&(k - 1)]) + 1;
        wf_succ.offsets.insert(k, max!(max_ins_sub, wf_prec.offsets[&(k + 1)]));

        /*print!("\t\t - next_wavefront[{}]->next_offsets[{}]: {}\n", distance, k, wf_succ.offsets[(k + (-wf_succ.lo)) as usize]);
        print!("\t\t - wavefront[{}]->offsets[{}]: {}\n", distance - 1, k, wf_prec.offsets[(k + (-lo)) as usize]);*/
    }

    let wf_prec_offset_hi = wf_prec.offsets[&(wf_prec.hi)];

    // Loop peeling (k=hi)
    let top_lower_ins = if wf_prec.lo <= (wf_prec.hi - 1) { wf_prec.offsets[&(wf_prec.hi - 1)] } else { -1 };
    wf_succ.offsets.insert(wf_prec.hi, max!(wf_prec_offset_hi, top_lower_ins) + 1);

    // Loop peeling (k=hi+1); there is only the Lower DP cell (there are no Mid and Upper cells)
    wf_succ.offsets.insert(wf_prec.hi + 1, wf_prec_offset_hi + 1);

    /*print!("\t\thi    , next_wavefront[{}]->next_offsets[{}]: {}\n", distance, hi, wf_succ.offsets[(hi + (-wf_succ.lo)) as usize]);
    print!("\t\thi    , wavefront[{}]->offsets[{}]: {}\n", distance - 1, hi, wf_prec.offsets[(hi + (-wf_prec.lo)) as usize]);
    print!("\t\thi + 1, next_wavefront[{}]->next_offsets[{}]: {}\n", distance, hi + 1, wf_succ.offsets[(hi + 1 + (-wf_succ.lo)) as usize]);
    print!("\t\thi + 1, wavefront[{}]->offsets[{}]: {}\n", distance - 1, hi + 1, wf_prec.offsets[(hi + 1 + (-wf_prec.lo)) as usize]);*/
}

fn edit_wavefronts_align(
    wavefronts: &mut EditWavefrontsT,
    pattern: &[u8], pattern_length: usize,
    text: &[u8], text_length: usize,
) {
    // Parameters
    let target_k: isize = ewavefront_diagonal!(text_length as isize, pattern_length as isize);  // h - v
    let target_offset: EwfOffsetT = ewavefront_offset!(text_length, pattern) as EwfOffsetT;     // h

    let target_k_abs: usize = abs!(target_k) as usize;

    /*print!("edit_wavefronts_align\n");
    print!("\ttarget_k: {} == text_length ({}) - pattern_length ({})\n", target_k, text_length, pattern_length);
    print!("\ttarget_offset: {} == text_length ({})\n\n", target_offset, text_length);*/

    // Init wavefronts
    edit_wavefronts_allocate_wavefront(&mut wavefronts.wavefronts[0], 0, 0);;
    wavefronts.wavefronts_allocated += 1; // Next
    //print!("\t\tedit_wavefronts->wavefronts_allocated: {}\n", wavefronts.wavefronts_allocated);

    wavefronts.wavefronts[0].offsets.insert(0, 0);

    let mut target_distance: usize = wavefronts.max_distance;

    // Compute wavefronts for increasing distance
    for distance in 0..wavefronts.max_distance {
        // Extend diagonally each wavefront point
        edit_wavefronts_extend_wavefront(
            wavefronts.wavefronts.get_mut(distance).unwrap(),
            pattern, pattern_length,
            text, text_length,
            distance,
        );

        // Exit condition (the minimum distance is the absolute difference of the sequences' lengths aligned)
        if distance >= target_k_abs &&
            wavefronts.wavefronts[distance].offsets[&(target_k)] == target_offset {
            /*print!("Exit condition\n");
            print!("\tdistance ({}) >= target_k_abs ({})\n", distance, target_k_abs);
            print!("\twavefronts[{}]->offsets[{}] ({}) == target_offset ({})\n",
                   distance, target_k_abs,
                   wavefronts.wavefronts[distance].offsets[(target_k - wavefronts.wavefronts[distance].lo) as usize],
                   target_offset
            );*/

            target_distance = distance;
            break;
        }

        // Compute next wavefront starting point
        edit_wavefronts_compute_wavefront(
            wavefronts,
            pattern_length,
            text_length,
            distance + 1,
        );
    }

    // Backtrace wavefronts
    edit_wavefronts_backtrace(wavefronts, pattern, text, target_k, target_distance);
}

fn edit_wavefronts_clean(
    wavefronts: &mut EditWavefrontsT
) {
    for i in 0..wavefronts.wavefronts_allocated {
        wavefronts.wavefronts[i].offsets.clear();
    }
    wavefronts.wavefronts_allocated = 0;
}


fn edit_wavefronts_allocate_wavefront(
    wavefront: &mut EditWavefrontT,
    lo_base: isize, hi_base: isize,
) {
    // Compute limits
    let wavefront_length: usize = (hi_base - lo_base + 1) as usize;//if hi_base == lo_base && hi_base == 0 { 1 } else { 2 }) as usize; // (+1) for k=0

    // Configure offsets
    wavefront.lo = lo_base;
    wavefront.hi = hi_base;

    // Allocate offsets
    wavefront.offsets = FxHashMap::with_capacity_and_hasher(wavefront_length, Default::default());

    /*print!("\tedit_wavefronts_allocate_wavefront\n");
    print!("\t\twavefront_length: {}\n", wavefront_length);
    print!("\t\twavefronts[{}]->[lo_base, hi_base] == [{}, {}]\n", distance, lo_base, hi_base);*/
}

fn main() {
    // Buffers
    let pattern_mem = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\
    TCTTTACTCGCGCGTTGGAGAAATACAATAGTTCTTTACTCGCGCGTTGGAGAAATACAATAGTTCTTTACTCGCGCGTTGGAGAAATACAATAGTTCTTTACTCGCGCGTTGGAGAAATACAATAGT\
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX".as_bytes();
    let text_mem = "\
    YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\
    TCTATACTGCGCGTTTGGAGAAATAAAATAGTTCTATACTGCGCGTTTGGAGAAATAAAATAGTTCTATACTGCGCGTTTGGAGAAATAAAATAGTTCTATACTGCGCGTTTGGAGAAATAAAATAGT\
    YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY".as_bytes();

    // Pattern & Text (seq + 64 Xs/Ys)
    let pattern = &pattern_mem[64..];
    let text = &text_mem[64..];
    /*println!("pattern+XXX...: {}", pattern);
    println!("   text+YYY...: {}", text);*/

    // isize to avoid overflow operation in a - b when a < b and a usize and b usize
    let pattern_length: usize = pattern_mem.len() - 2 * 64;
    let text_length: usize = text_mem.len() - 2 * 64;
    let reps: usize = 10_000_00;//0_000_000;

    /*println!("pattern_length: {}", pattern_length);
    println!("   text_length: {}", text_length);*/

    // Init Wavefronts
    let mut wavefronts: EditWavefrontsT = EditWavefrontsT {
        // Dimensions
        pattern_length,
        text_length,
        max_distance: pattern_length + text_length,

        wavefronts: Vec::with_capacity(pattern_length + text_length),
        wavefronts_allocated: 0,

        // Allocate CIGAR
        edit_cigar: vec![0; pattern_length + text_length],
        edit_cigar_length: 0,
    };
    //edit_wavefronts_init()
    for _ in 0..wavefronts.max_distance {
        wavefronts.wavefronts.push(
            EditWavefrontT { lo: 0, hi: 0, offsets: FxHashMap::default() }
        );
    }

    for _ in 0..reps {
        edit_wavefronts_clean(&mut wavefronts);
        edit_wavefronts_align(&mut wavefronts, pattern, pattern_length, text, text_length);
    }

    // Two ways to display the CIGAR string
    // 1)
    //for i in (0..wavefronts.edit_cigar_length).rev() {
    //    print!("{}", wavefronts.edit_cigar[i] as char);
    //}
    // 2)
    //wavefronts.edit_cigar.truncate(wavefronts.edit_cigar_length);
    //wavefronts.edit_cigar.reverse();
    //debug_assert!(&wavefronts.edit_cigar == &"MMMXMMMMDMMMMMMMIMMMMMMMMMXMMMMMMMMMXMMMMDMMMMMMMIMMMMMMMMMXMMMMMMMMMXMMMMDMMMMMMMIMMMMMMMMMXMMMMMMMMMXMMMMDMMMMMMMIMMMMMMMMMXMMMMMM".as_bytes());
    //println!("{}", String::from_utf8(wavefronts.edit_cigar).unwrap());
}
