/*
	 Copyright 2017, Daniel Valenzuela <dvalenzu@cs.helsinki.fi>

	 This file is part of CHIC aligner.

	 CHIC aligner is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.

	 CHIC aligner is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with CHIC aligner.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SRC_BASIC_H_
#define SRC_BASIC_H_

#include <stdlib.h>
#include <sys/resource.h>

// This is a trick to avoid warnings for unused variables that are used only for assert:
#ifdef NDEBUG
#define ASSERT(x) do { (void)sizeof(x);} while (0)
#else
#include <assert.h>
#define ASSERT(x) assert(x)
#endif

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

enum class KernelType { BWA, BOWTIE2, BLAST, FMI };
enum class InputType { PLAIN, FQ };
enum class SecondaryReportType { ALL, LZ, NONE };
enum class LZMethod {IN_MEMORY, EXTERNAL_MEMORY, RLZ, INPUT};

typedef struct {
	char * input_filename;
	char * output_filename;
	char * input_lz_filename;
	LZMethod lz_method;
	int n_threads;
	int mem_limit_MB;
	int rlz_ref_len_MB;
	uint max_query_len;
	uint max_edit_distance;
	KernelType kernel_type;
	int verbose;
} BuildParameters;

#endif  // SRC_BASIC_H_
