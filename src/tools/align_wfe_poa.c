/*
 *                             The MIT License
 *
 * Wavefront Alignments Algorithms
 * Copyright (c) 2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 * This file is part of WFPOA.
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
 * PROJECT: Partial Order Alignment Wavefront Alignment (WFPOA)
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "utils/text_dag.h"
#include "system/mm_allocator.h"
#include "edit/edit_dp_poa.h"
#include "edit/wfe_poa/edit_wavefront_poa.h"
#include "edit/wfe_poa/edit_wavefront_poa_align.h"

int main(int argc,char* argv[]) {
  // Text-DAG
  text_dag_t* text_dag = text_dag_example1();
  // Pattern
  //char* pattern_buffer = "YACTGTACTY"; // (0)3M(2)2M(3)3M
  //char* pattern_buffer = "YAGTGGAGTY"; // (0)1M1X1M(2)1M1X(3)1M1X1M
  //char* pattern_buffer = "YACGTATY"; // (0)2M1I(2)2M(3)1M1I1M
  //char* pattern_buffer = "YGTY"; // (0)3I(2)2M(3)3I
  char* pattern_buffer = "YCTACGACY"; // (0)1I2M(1)2M2I1M(3)2M1I
  /* */
  char* pattern = pattern_buffer + 1;
  const int pattern_length = strlen(pattern_buffer) - 2;

  // MM-Allocator
  mm_allocator_t* const mm_allocator = mm_allocator_new(BUFFER_SIZE_8M);
  // Allocate CIGAR
  cigar_t cigar;
  cigar_allocate(&cigar,pattern_length,10000,mm_allocator);

//  // Compute POA using dynamic programming
//  edit_dp_poa_compute(pattern,pattern_length,text_dag,&cigar,mm_allocator);

  // Compute POA using WFE-POA
  edit_wavefront_poa_t* const wavefront_poa = edit_wavefront_poa_new(mm_allocator);
  edit_wavefront_poa_align(wavefront_poa,pattern,pattern_length,text_dag,&cigar);

  // Display backtrace
  cigar_print(stderr,&cigar);

  // Free
  cigar_free(&cigar,mm_allocator);
  edit_wavefront_poa_delete(wavefront_poa);
  mm_allocator_delete(mm_allocator);
  text_dag_delete(text_dag);
}









