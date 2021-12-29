/*
 *                             The MIT License
 *
 * Wavefront Alignments Algorithms
 * Copyright (c) 2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 * This file is part of Wavefront Alignments Algorithms.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * PROJECT: Wavefront Alignments Algorithms
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Cigar data-structure (match/mismatch/insertion/deletion)
 */

#include "cigar.h"

/*
 * Setup
 */
void cigar_allocate(
    cigar_t* const cigar,
    const int pattern_length,
    const int text_length,
    mm_allocator_t* const mm_allocator) {
  cigar->max_operations = pattern_length+text_length;
  cigar->operations = mm_allocator_malloc(mm_allocator,cigar->max_operations);
  cigar->begin_offset = cigar->max_operations;
  cigar->end_offset = cigar->max_operations;
  cigar->score = INT32_MIN;
}
void cigar_clear(
    cigar_t* const cigar) {
  cigar->begin_offset = cigar->max_operations;
  cigar->end_offset = cigar->max_operations;
  cigar->score = INT32_MIN;
}
void cigar_free(
    cigar_t* const cigar,
    mm_allocator_t* const mm_allocator) {
  mm_allocator_free(mm_allocator,cigar->operations);
}
/*
 * Accessors
 */
int cigar_compute_num_matches(
    cigar_t* const cigar) {
  int i, num_matches=0;
  for (i=cigar->begin_offset;i<cigar->end_offset;++i) {
    num_matches += (cigar->operations[i]=='M');
  }
  return num_matches;
}
void cigar_compute_mismatches(
    char* const pattern,
    const int pattern_length,
    char* const text,
    const int text_length,
    cigar_t* const cigar) {
  // Refine adding mismatches
  int i, p=0, t=0;
  for (i=cigar->begin_offset;i<cigar->end_offset;++i) {
    // Check limits
    if (p >= pattern_length || t >= text_length) break;
    switch (cigar->operations[i]) {
      case 'M':
        cigar->operations[i] = (pattern[p]==text[t]) ? 'M' : 'X';
        ++p; ++t;
        break;
      case 'I':
        ++t;
        break;
      case 'D':
        ++p;
        break;
      default:
        fprintf(stderr,"Wrong edit operation\n");
        exit(1);
        break;
    }
  }
  while (p < pattern_length) { cigar->operations[i++] = 'D'; ++p; };
  while (t < text_length) { cigar->operations[i++] = 'I'; ++t; };
  cigar->end_offset = i;
  cigar->operations[cigar->end_offset] = '\0';
}
void cigar_add_leading_insertion(
    cigar_t* const cigar,
    const int length) {
  char* const operations = cigar->operations;
  int i;
  for (i=0;i<length;++i) {
    --(cigar->begin_offset);
    operations[cigar->begin_offset] = 'I';
  }
}
void cigar_add_leading_deletion(
    cigar_t* const cigar,
    const int length) {
  char* const operations = cigar->operations;
  int i;
  for (i=0;i<length;++i) {
    --(cigar->begin_offset);
    operations[cigar->begin_offset] = 'D';
  }
}
void cigar_add_segment(
    cigar_t* const cigar,
    const int segment_idx) {
  --(cigar->begin_offset);
  cigar->operations[cigar->begin_offset] = (char)(segment_idx + 48);
}
/*
 * Score
 */
int cigar_score_edit(
    cigar_t* const cigar) {
  int score = 0, i;
  for (i=cigar->begin_offset;i<cigar->end_offset;++i) {
    switch (cigar->operations[i]) {
      case 'M': break;
      case 'X':
      case 'D':
      case 'I': ++score; break;
      default: return INT_MIN;
    }
  }
  return score;
}
/*
 * Utils
 */
int cigar_cmp(
    cigar_t* const cigar_a,
    cigar_t* const cigar_b) {
  // Compare lengths
  const int length_cigar_a = cigar_a->end_offset - cigar_a->begin_offset;
  const int length_cigar_b = cigar_b->end_offset - cigar_b->begin_offset;
  if (length_cigar_a != length_cigar_b) return length_cigar_a - length_cigar_b;
  // Compare operations
  char* const operations_a = cigar_a->operations + cigar_a->begin_offset;
  char* const operations_b = cigar_b->operations + cigar_b->begin_offset;
  int i;
  for (i=0;i<length_cigar_a;++i) {
    if (operations_a[i] != operations_b[i]) {
      return operations_a[i] - operations_b[i];
    }
  }
  // Equal
  return 0;
}
void cigar_copy(
    cigar_t* const cigar_dst,
    cigar_t* const cigar_src) {
  cigar_dst->max_operations = cigar_src->max_operations;
  cigar_dst->begin_offset = cigar_src->begin_offset;
  cigar_dst->end_offset = cigar_src->end_offset;
  cigar_dst->score = cigar_src->score;
  memcpy(cigar_dst->operations+cigar_src->begin_offset,
         cigar_src->operations+cigar_src->begin_offset,
         cigar_src->end_offset-cigar_src->begin_offset);
}
bool cigar_check_alignment(
    FILE* const stream,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    cigar_t* const cigar,
    const bool verbose) {
  // Parameters
  char* const operations = cigar->operations;
  // Traverse CIGAR
  int pattern_pos=0, text_pos=0, i;
  for (i=cigar->begin_offset;i<cigar->end_offset;++i) {
    switch (operations[i]) {
      case 'M':
        // Check match
        if (pattern[pattern_pos] != text[text_pos]) {
          if (verbose) {
            fprintf(stream,
                "Align Check. Alignment not matching (pattern[%d]=%c != text[%d]=%c)\n",
                pattern_pos,pattern[pattern_pos],text_pos,text[text_pos]);
          }
          return false;
        }
        ++pattern_pos;
        ++text_pos;
        break;
      case 'X':
        // Check mismatch
        if (pattern[pattern_pos] == text[text_pos]) {
          if (verbose) {
            fprintf(stream,
                "Align Check. Alignment not mismatching (pattern[%d]=%c == text[%d]=%c)\n",
                pattern_pos,pattern[pattern_pos],text_pos,text[text_pos]);
          }
          return false;
        }
        ++pattern_pos;
        ++text_pos;
        break;
      case 'I':
        ++text_pos;
        break;
      case 'D':
        ++pattern_pos;
        break;
      default:
        fprintf(stderr,"CIGAR check. Unknown edit operation '%c'\n",operations[i]);
        exit(1);
        break;
    }
  }
  // Check alignment length
  if (pattern_pos != pattern_length) {
    if (verbose) {
      fprintf(stream,
          "Align Check. Alignment incorrect length (pattern-aligned=%d,pattern-length=%d)\n",
          pattern_pos,pattern_length);
    }
    return false;
  }
  if (text_pos != text_length) {
    if (verbose) {
      fprintf(stream,
          "Align Check. Alignment incorrect length (text-aligned=%d,text-length=%d)\n",
          text_pos,text_length);
    }
    return false;
  }
  // OK
  return true;
}
/*
 * Display
 */
void cigar_print(
    FILE* const stream,
    cigar_t* const cigar) {
  char last_op = '\0';
  int i, last_op_length = -1;
  for (i=cigar->begin_offset;i<cigar->end_offset;++i) {
    if ('0' <= cigar->operations[i] && cigar->operations[i] <= '9') {
      if (last_op_length != -1) fprintf(stream,"%d%c",last_op_length,last_op);
      fprintf(stream,"(%c)",cigar->operations[i]);
      last_op = '\0';
      last_op_length = -1;
    } else {
      if (cigar->operations[i] == last_op) {
        ++last_op_length;
      } else {
        if (last_op_length != -1) fprintf(stream,"%d%c",last_op_length,last_op);
        last_op = cigar->operations[i];
        last_op_length = 1;
      }
    }
  }
  if (last_op_length != -1) fprintf(stream,"%d%c",last_op_length,last_op);
}
void cigar_print_pretty(
    FILE* const stream,
    const char* const pattern,
    const int pattern_length,
    const char* const text,
    const int text_length,
    cigar_t* const cigar,
    mm_allocator_t* const mm_allocator) {
  // Parameters
  char* const operations = cigar->operations;
  // Allocate alignment buffers
  const int max_buffer_length = text_length+pattern_length+1;
  char* const pattern_alg = mm_allocator_calloc(mm_allocator,max_buffer_length,char,true);
  char* const ops_alg = mm_allocator_calloc(mm_allocator,max_buffer_length,char,true);
  char* const text_alg = mm_allocator_calloc(mm_allocator,max_buffer_length,char,true);
  // Compute alignment buffers
  int i, alg_pos = 0, pattern_pos = 0, text_pos = 0;
  for (i=cigar->begin_offset;i<cigar->end_offset;++i) {
    switch (operations[i]) {
      case 'M':
        if (pattern[pattern_pos] != text[text_pos]) {
          pattern_alg[alg_pos] = pattern[pattern_pos];
          ops_alg[alg_pos] = 'X';
          text_alg[alg_pos++] = text[text_pos];
        } else {
          pattern_alg[alg_pos] = pattern[pattern_pos];
          ops_alg[alg_pos] = '|';
          text_alg[alg_pos++] = text[text_pos];
        }
        pattern_pos++; text_pos++;
        break;
      case 'X':
        if (pattern[pattern_pos] != text[text_pos]) {
          pattern_alg[alg_pos] = pattern[pattern_pos++];
          ops_alg[alg_pos] = ' ';
          text_alg[alg_pos++] = text[text_pos++];
        } else {
          pattern_alg[alg_pos] = pattern[pattern_pos++];
          ops_alg[alg_pos] = 'X';
          text_alg[alg_pos++] = text[text_pos++];
        }
        break;
      case 'I':
        pattern_alg[alg_pos] = '-';
        ops_alg[alg_pos] = ' ';
        text_alg[alg_pos++] = text[text_pos++];
        break;
      case 'D':
        pattern_alg[alg_pos] = pattern[pattern_pos++];
        ops_alg[alg_pos] = ' ';
        text_alg[alg_pos++] = '-';
        break;
      default:
        break;
    }
  }
  i=0;
  while (pattern_pos < pattern_length) {
    pattern_alg[alg_pos+i] = pattern[pattern_pos++];
    ops_alg[alg_pos+i] = '?';
    ++i;
  }
  i=0;
  while (text_pos < text_length) {
    text_alg[alg_pos+i] = text[text_pos++];
    ops_alg[alg_pos+i] = '?';
    ++i;
  }
  // Print alignment pretty
  fprintf(stream,"      PRETTY.ALIGNMENT\t");
  cigar_print(stderr,cigar);
  fprintf(stream,"\n");
  fprintf(stream,"      PATTERN    %s\n",pattern_alg);
  fprintf(stream,"                 %s\n",ops_alg);
  fprintf(stream,"      TEXT       %s\n",text_alg);
  // Free
  mm_allocator_free(mm_allocator,pattern_alg);
  mm_allocator_free(mm_allocator,ops_alg);
  mm_allocator_free(mm_allocator,text_alg);
}


