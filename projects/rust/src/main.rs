use std::cmp::max;

type EwfOffsetT = i16;

struct EditWavefrontT {
    lo: isize,
    // Effective lowest diagonal (inclusive)
    hi: isize,
    // Effective highest diagonal (inclusive)
    offsets: Vec<EwfOffsetT>, // Offsets

    lo_base: isize,
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
    edit_cigar_length : usize,
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

    let wavefront = &wavefronts_slice[target_distance];

    let mut offset = wavefronts_slice[target_distance].offsets[(k - wavefront.lo_base) as usize];

    wavefronts.edit_cigar_length = 0;

    while distance > 0 {
        /*print!("\tdistance: {}\n", distance);
        print!("\tk: {}\n", k);*/

        // Fetch
        let wavefront = &wavefronts_slice[distance - 1];
        let lo_base = wavefront.lo_base;

        /*print!("\tdistance - 1: {}\n", distance - 1);
        print!("\t\twavefront->lo: {}\n", wavefront.lo);
        print!("\t\twavefront->hi: {}\n", wavefront.hi);
        if wavefront.lo <= k - 1 && k - 1 <= wavefront.hi {
            print!("\t\toffsets[{}]: {}\n", k - 1, wavefront.offsets[(k - 1 + (-lo_base)) as usize]);
        }
        print!("\t\toffset[{}]: {}\n", k - 1, offset);
        if wavefront.lo <= k + 1 && k + 1 <= wavefront.hi {
            print!("\t\toffsets[{}]: {}\n", k + 1, wavefront.offsets[(k + 1 + (-lo_base)) as usize]);
        }*/

        // Traceback operation
        if wavefront.lo <= k + 1 && k + 1 <= wavefront.hi && offset == wavefront.offsets[(k + 1 + (-lo_base)) as usize] {
            wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'D';
            wavefronts.edit_cigar_length += 1;
            k += 1;
            distance -= 1;
        } else if wavefront.lo <= k - 1 && k - 1 <= wavefront.hi && offset == wavefront.offsets[(k - 1 + (-lo_base)) as usize] + 1 {
            wavefronts.edit_cigar[wavefronts.edit_cigar_length] = b'I';
            wavefronts.edit_cigar_length += 1;
            k -= 1;
            offset -= 1;
            distance -= 1;
        } else if wavefront.lo <= k && k <= wavefront.hi && offset == wavefront.offsets[(k + (-lo_base)) as usize] + 1 {
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
    _distance : usize
) {
    // Parameters
    let k_min = wavefront.lo;
    let k_max = wavefront.hi;
    let lo_base = wavefront.lo_base;

    // Extend diagonally each wavefront point
    for k in k_min..=k_max {
        let mut v = (wavefront.offsets[(k + (-lo_base)) as usize] as isize - k) as usize; //EWAVEFRONT_V(k, offsets[k]); // offsets[k]-k
        let mut h = wavefront.offsets[(k + (-lo_base)) as usize] as usize;     //EWAVEFRONT_H(k, offsets[k]); // offsets[k]

        /*print!("\tedit_wavefronts_extend_wavefront\n");
        print!("\t\tk: {}\n", k);
        print!("\t\toffsets[{}]: {}\n", k, wavefront.offsets[(k + (-lo_base)) as usize]);
        print!("\t\t\t(v, h) == ({}, {}) ==> ({}, {})\n", v, h, pattern[v] as char, text[h] as char);*/
        while v < pattern_length && h < text_length && pattern[v] == text[h] {
            wavefront.offsets[(k + (-lo_base)) as usize] += 1;

            v += 1;
            h += 1;

            /*print!("\t\t\twavefronts[{}]->offsets[{}]: {}\n", _distance, k, wavefront.offsets[(k + (-lo_base + 0)) as usize]);
            print!("\t\t\t(v, h) == ({}, {}) ==> ({}, {})\n", v, h, pattern[v] as char, text[h] as char);*/
        }
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
    let wavefront_next_wavefront = &mut wavefronts.wavefronts[distance - 1..=distance];

    //let wavefront: &EditWavefrontT = &wavefronts.wavefronts[distance - 1];
    let hi = wavefront_next_wavefront[0].hi;
    let lo = wavefront_next_wavefront[0].lo;
    let lo_base = wavefront_next_wavefront[0].lo_base;

    edit_wavefronts_allocate_wavefront(
        &mut wavefront_next_wavefront[1], lo - 1, hi + 1,
    );
    // Allocate wavefront
    wavefronts.wavefronts_allocated += 1; // Next
    //wavefronts.wavefronts_allocated += 1;
    /*print!("\t\tedit_wavefronts->wavefronts_allocated: {}\n", wavefronts.wavefronts_allocated);
    print!("\t---------------\n");*/

    // Fetch offsets
    let lo_base_next = wavefront_next_wavefront[1].lo_base;

    // Loop peeling (k=lo-1)
    wavefront_next_wavefront[1].offsets[(lo - 1 + (-lo_base_next)) as usize] = wavefront_next_wavefront[0].offsets[(lo + (-lo_base)) as usize];

    // Loop peeling (k=lo)
    let bottom_upper_del = if (lo + 1) <= hi { wavefront_next_wavefront[0].offsets[(lo + 1 + (-lo_base)) as usize] } else { -1 };
    wavefront_next_wavefront[1].offsets[(lo + (-lo_base_next)) as usize] = max(wavefront_next_wavefront[0].offsets[(lo + (-lo_base)) as usize] + 1, bottom_upper_del);

    /*print!("\t\tlo - 1, next_wavefront[{}]->next_offsets[{}]: {}\n", distance, lo - 1, wavefront_next_wavefront[1].offsets[(lo - 1 + (-lo_base_next)) as usize]);
    //print!("\t\tlo - 1, wavefront[{}]->offsets[{}]: {}\n", distance - 1, lo - 1, wavefront_next_wavefront[0].offsets[(lo - 1 + (-lo_base)) as usize]);
    print!("\t\tlo    , next_wavefront[{}]->next_offsets[{}]: {}\n", distance, lo, wavefront_next_wavefront[1].offsets[(lo + (-lo_base_next)) as usize]);
    print!("\t\tlo    , wavefront[{}]->offsets[{}]: {}\n", distance - 1, lo, wavefront_next_wavefront[0].offsets[(lo + (-lo_base)) as usize]);*/

    // Compute next wavefront starting point
    for k in (lo+1)..=(hi - 1) {
        let max_ins_sub = max(wavefront_next_wavefront[0].offsets[(k + (-lo_base)) as usize], wavefront_next_wavefront[0].offsets[(k - 1 + (-lo_base)) as usize]) + 1;
        wavefront_next_wavefront[1].offsets[(k + (-lo_base_next)) as usize] = max(max_ins_sub, wavefront_next_wavefront[0].offsets[(k + 1 + (-lo_base)) as usize]);

        /*print!("\t\t - next_wavefront[{}]->next_offsets[{}]: {}\n", distance, k, wavefront_next_wavefront[1].offsets[(k + (-lo_base_next)) as usize]);
        print!("\t\t - wavefront[{}]->offsets[{}]: {}\n", distance - 1, k, wavefront_next_wavefront[0].offsets[(k + (-lo_base)) as usize]);*/
    }

    // Loop peeling (k=hi)
    let top_lower_ins = if lo <= (hi - 1) { wavefront_next_wavefront[0].offsets[(hi - 1 + (-lo_base)) as usize] } else { -1 };
    wavefront_next_wavefront[1].offsets[(hi + (-lo_base_next)) as usize] = max(wavefront_next_wavefront[0].offsets[(hi + (-lo_base)) as usize], top_lower_ins) + 1;

    // Loop peeling (k=hi+1)
    wavefront_next_wavefront[1].offsets[(hi + 1 + (-lo_base_next)) as usize] = wavefront_next_wavefront[0].offsets[(hi + (-lo_base)) as usize] + 1;

    /*print!("\t\thi    , next_wavefront[{}]->next_offsets[{}]: {}\n", distance, hi, wavefront_next_wavefront[1].offsets[(hi + (-lo_base_next)) as usize]);
    print!("\t\thi    , wavefront[{}]->offsets[{}]: {}\n", distance - 1, hi, wavefront_next_wavefront[0].offsets[(hi + (-lo_base)) as usize]);
    print!("\t\thi + 1, next_wavefront[{}]->next_offsets[{}]: {}\n", distance, hi + 1, wavefront_next_wavefront[1].offsets[(hi + 1 + (-lo_base_next)) as usize]);
    print!("\t\thi + 1, wavefront[{}]->offsets[{}]: {}\n", distance - 1, hi + 1, wavefront_next_wavefront[0].offsets[(hi + 1 + (-lo_base)) as usize]);*/
}

fn edit_wavefronts_align(
    wavefronts: &mut EditWavefrontsT,
    pattern: &[u8], pattern_length: usize,
    text: &[u8], text_length: usize,
) {
// Parameters
    let max_distance: usize = pattern_length + text_length;
    let target_k: isize = text_length as isize - pattern_length as isize ;//EWAVEFRONT_DIAGONAL(text_length, pattern_length); // h - v
    let target_k_abs: usize = target_k.abs() as usize;
    let target_offset: EwfOffsetT = text_length as EwfOffsetT;//EWAVEFRONT_OFFSET(text_length, pattern_length); // h

    /*print!("edit_wavefronts_align\n");
    print!("\ttarget_k: {} == text_length ({}) - pattern_lenght ({})\n", target_k, text_length, pattern_length);
    print!("\ttarget_offset: {} == text_length ({})\n\n", target_offset, text_length);*/

    // Init wavefronts
    edit_wavefronts_allocate_wavefront(&mut wavefronts.wavefronts[0], 0, 0);
    wavefronts.wavefronts_allocated += 1; // Next

    //print!("\t\tedit_wavefronts->wavefronts_allocated: {}\n", wavefronts.wavefronts_allocated);

    let mut target_distance : usize = max_distance;

    // Compute wavefronts for increasing distance
    for distance in 0..max_distance {
        // Extend diagonally each wavefront point
        edit_wavefronts_extend_wavefront(
            &mut wavefronts.wavefronts[distance],
            pattern, pattern_length,
            text, text_length,
            distance
        );

        // Exit condition
        if target_k_abs <= distance &&
            wavefronts.wavefronts[distance].offsets[(target_k - wavefronts.wavefronts[distance].lo_base) as usize] == target_offset {
            /*print!("Exit condition\n");
            print!("\ttarget_k_abs ({}) <= distance ({})\n", target_k_abs, distance);
            print!("\twavefronts[{}]->offsets[{}] ({}) == target_offset ({})\n",
                   distance, target_k_abs,
                   wavefronts.wavefronts[distance].offsets[(target_k - wavefronts.wavefronts[distance].lo_base) as usize],
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
    let wavefront_length: usize = (hi_base - lo_base + 2) as usize;//if hi_base == lo_base && hi_base == 0 { 1 } else { 2 }) as usize; // (+1) for k=0

    // Configure offsets
    wavefront.lo = lo_base;
    wavefront.hi = hi_base;
    wavefront.lo_base = lo_base;

    // Allocate offsets
    wavefront.offsets = vec![0; wavefront_length];

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
    let reps: usize = 10000000;

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
        edit_cigar_length: 0
    };
    //edit_wavefronts_init()
    for _ in 0..wavefronts.max_distance {
        wavefronts.wavefronts.push(
            EditWavefrontT { lo: 0, hi: 0, lo_base: 0, offsets: Vec::new() }
        );
    }

    for _ in 0..reps {
        edit_wavefronts_clean(&mut wavefronts);
        edit_wavefronts_align(&mut wavefronts, pattern, pattern_length, text, text_length);
    }

    // Two ways to visualize the CIGAR string
    // 1)
    //for i in (0..wavefronts.edit_cigar_length).rev() {
    //    print!("{}", wavefronts.edit_cigar[i] as char);
    //}
    // 2)
    //wavefronts.edit_cigar.reverse();
    //println!("{}", String::from_utf8(wavefronts.edit_cigar).unwrap());
}
