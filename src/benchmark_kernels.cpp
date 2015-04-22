/* The MIT License (MIT)
 *
 * Copyright (c) 2014 Microsoft
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
 */

/**
 * @file
 * 
 * @brief Implementation file for benchmark kernel functions for doing the actual work we care about. :)
 *
 * Optimization tricks include:
 *   - UNROLL macros to manual loop unrolling. This reduces the relative branch overhead of the loop.
 *     We don't want to benchmark loops, we want to benchmark memory! But unrolling too much can hurt
 *	   code size and instruction locality, potentially decreasing I-cache utilization and causing extra overheads.
 *	   This is why we allow multiple unroll lengths at compile-time.
 *   - volatile keyword to prevent compiler from optimizing the code and removing instructions that we need.
 *     The compiler is too smart for its own good!
 */

//Headers
#include <benchmark_kernels.h>
#include <common.h>

//Libraries
#include <iostream>
#include <random>
#include <algorithm>
#include <time.h>
#if defined(__gnu_linux__) && defined(ARCH_INTEL_X86_64)
#include <immintrin.h> //for Intel AVX intrinsics
#endif

#ifdef ARCH_INTEL_X86_64
#define my_32b_set_128b_word(a, b, c, d) _mm_set_epi32x(a, b, c, d)
#define my_32b_set_256b_word(a, b, c, d, e, f, g, h) _mm256_set_epi32x(a, b, c, d, e, f, g, h)
#define my_64b_set_128b_word(a, b) _mm_set_epi64x(a, b)
#define my_64b_set_256b_word(a, b, c, d) _mm256_set_epi64x(a, b, c, d)

#define my_32b_extractLSB_128b(w) _mm_extract_epi32(w, 0)
#define my_32b_extractLSB_256b(w) _mm256_extract_epi32(w, 0)
#define my_64b_extractLSB_128b(w) _mm_extract_epi64(w, 0)
#define my_64b_extractLSB_256b(w) _mm256_extract_epi64(w, 0)
#endif

#ifdef ARCH_ARM
#error TODO: ARM intrinsics
#endif

using namespace xmem;

bool xmem::determineSequentialKernel(rw_mode_t rw_mode, chunk_size_t chunk_size, int32_t stride_size, SequentialFunction* kernel_function, SequentialFunction* dummy_kernel_function) {
	switch (rw_mode) {
		case READ:
			switch (chunk_size) {
				case CHUNK_32b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialRead_Word32;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word32;
							return true;
						case -1:
							*kernel_function = &revSequentialRead_Word32;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word32;
							return true;
						case 2:
							*kernel_function = &forwStride2Read_Word32;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word32;
							return true;
						case -2:
							*kernel_function = &revStride2Read_Word32;
							*dummy_kernel_function = &dummy_revStride2Loop_Word32;
							return true;
						case 4:
							*kernel_function = &forwStride4Read_Word32;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word32;
							return true;
						case -4:
							*kernel_function = &revStride4Read_Word32;
							*dummy_kernel_function = &dummy_revStride4Loop_Word32;
							return true;
						case 8:
							*kernel_function = &forwStride8Read_Word32;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word32;
							return true;
						case -8:
							*kernel_function = &revStride8Read_Word32;
							*dummy_kernel_function = &dummy_revStride8Loop_Word32;
							return true;
						case 16:
							*kernel_function = &forwStride16Read_Word32;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word32;
							return true;
						case -16:
							*kernel_function = &revStride16Read_Word32;
							*dummy_kernel_function = &dummy_revStride16Loop_Word32;
							return true;
						default:
							return false;
							return true;
					}
					return true;
#ifdef HAS_WORD_64
				case CHUNK_64b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialRead_Word64;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word64;
							return true;
						case -1:
							*kernel_function = &revSequentialRead_Word64;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word64;
							return true;
						case 2:
							*kernel_function = &forwStride2Read_Word64;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word64;
							return true;
						case -2:
							*kernel_function = &revStride2Read_Word64;
							*dummy_kernel_function = &dummy_revStride2Loop_Word64;
							return true;
						case 4:
							*kernel_function = &forwStride4Read_Word64;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word64;
							return true;
						case -4:
							*kernel_function = &revStride4Read_Word64;
							*dummy_kernel_function = &dummy_revStride4Loop_Word64;
							return true;
						case 8:
							*kernel_function = &forwStride8Read_Word64;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word64;
							return true;
						case -8:
							*kernel_function = &revStride8Read_Word64;
							*dummy_kernel_function = &dummy_revStride8Loop_Word64;
							return true;
						case 16:
							*kernel_function = &forwStride16Read_Word64;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word64;
							return true;
						case -16:
							*kernel_function = &revStride16Read_Word64;
							*dummy_kernel_function = &dummy_revStride16Loop_Word64;
							return true;
						default:
							return false;
					}
					return true;
#endif
#ifdef HAS_WORD_128
				case CHUNK_128b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialRead_Word128;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word128;
							return true;
						case -1:
							*kernel_function = &revSequentialRead_Word128;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word128;
							return true;
						case 2:
							*kernel_function = &forwStride2Read_Word128;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word128;
							return true;
						case -2:
							*kernel_function = &revStride2Read_Word128;
							*dummy_kernel_function = &dummy_revStride2Loop_Word128;
							return true;
						case 4:
							*kernel_function = &forwStride4Read_Word128;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word128;
							return true;
						case -4:
							*kernel_function = &revStride4Read_Word128;
							*dummy_kernel_function = &dummy_revStride4Loop_Word128;
							return true;
						case 8:
							*kernel_function = &forwStride8Read_Word128;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word128;
							return true;
						case -8:
							*kernel_function = &revStride8Read_Word128;
							*dummy_kernel_function = &dummy_revStride8Loop_Word128;
							return true;
						case 16:
							*kernel_function = &forwStride16Read_Word128;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word128;
							return true;
						case -16:
							*kernel_function = &revStride16Read_Word128;
							*dummy_kernel_function = &dummy_revStride16Loop_Word128;
							return true;
						default:
							return false;
					}
					return true;
#endif
#ifdef HAS_WORD_256
				case CHUNK_256b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialRead_Word256;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word256;
							return true;
						case -1:
							*kernel_function = &revSequentialRead_Word256;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word256;
							return true;
						case 2:
							*kernel_function = &forwStride2Read_Word256;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word256;
							return true;
						case -2:
							*kernel_function = &revStride2Read_Word256;
							*dummy_kernel_function = &dummy_revStride2Loop_Word256;
							return true;
						case 4:
							*kernel_function = &forwStride4Read_Word256;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word256;
							return true;
						case -4:
							*kernel_function = &revStride4Read_Word256;
							*dummy_kernel_function = &dummy_revStride4Loop_Word256;
							return true;
						case 8:
							*kernel_function = &forwStride8Read_Word256;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word256;
							return true;
						case -8:
							*kernel_function = &revStride8Read_Word256;
							*dummy_kernel_function = &dummy_revStride8Loop_Word256;
							return true;
						case 16:
							*kernel_function = &forwStride16Read_Word256;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word256;
							return true;
						case -16:
							*kernel_function = &revStride16Read_Word256;
							*dummy_kernel_function = &dummy_revStride16Loop_Word256;
							return true;
						default:
							return false;
					}
					return true;
#endif

				default:
					return false;
			}
			return true;

		case WRITE:
			switch (chunk_size) {
				case CHUNK_32b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialWrite_Word32;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word32;
							return true;
						case -1:
							*kernel_function = &revSequentialWrite_Word32;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word32;
							return true;
						case 2:
							*kernel_function = &forwStride2Write_Word32;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word32;
							return true;
						case -2:
							*kernel_function = &revStride2Write_Word32;
							*dummy_kernel_function = &dummy_revStride2Loop_Word32;
							return true;
						case 4:
							*kernel_function = &forwStride4Write_Word32;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word32;
							return true;
						case -4:
							*kernel_function = &revStride4Write_Word32;
							*dummy_kernel_function = &dummy_revStride4Loop_Word32;
							return true;
						case 8:
							*kernel_function = &forwStride8Write_Word32;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word32;
							return true;
						case -8:
							*kernel_function = &revStride8Write_Word32;
							*dummy_kernel_function = &dummy_revStride8Loop_Word32;
							return true;
						case 16:
							*kernel_function = &forwStride16Write_Word32;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word32;
							return true;
						case -16:
							*kernel_function = &revStride16Write_Word32;
							*dummy_kernel_function = &dummy_revStride16Loop_Word32;
							return true;
						default:
							return false;
					}
					return true;
#ifdef HAS_WORD_64
				case CHUNK_64b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialWrite_Word64;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word64;
							return true;
						case -1:
							*kernel_function = &revSequentialWrite_Word64;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word64;
							return true;
						case 2:
							*kernel_function = &forwStride2Write_Word64;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word64;
							return true;
						case -2:
							*kernel_function = &revStride2Write_Word64;
							*dummy_kernel_function = &dummy_revStride2Loop_Word64;
							return true;
						case 4:
							*kernel_function = &forwStride4Write_Word64;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word64;
							return true;
						case -4:
							*kernel_function = &revStride4Write_Word64;
							*dummy_kernel_function = &dummy_revStride4Loop_Word64;
							return true;
						case 8:
							*kernel_function = &forwStride8Write_Word64;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word64;
							return true;
						case -8:
							*kernel_function = &revStride8Write_Word64;
							*dummy_kernel_function = &dummy_revStride8Loop_Word64;
							return true;
						case 16:
							*kernel_function = &forwStride16Write_Word64;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word64;
							return true;
						case -16:
							*kernel_function = &revStride16Write_Word64;
							*dummy_kernel_function = &dummy_revStride16Loop_Word64;
							return true;
						default:
							return false;
					}
					return true;
#endif
#ifdef HAS_WORD_128
				case CHUNK_128b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialWrite_Word128;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word128;
							return true;
						case -1:
							*kernel_function = &revSequentialWrite_Word128;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word128;
							return true;
						case 2:
							*kernel_function = &forwStride2Write_Word128;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word128;
							return true;
						case -2:
							*kernel_function = &revStride2Write_Word128;
							*dummy_kernel_function = &dummy_revStride2Loop_Word128;
							return true;
						case 4:
							*kernel_function = &forwStride4Write_Word128;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word128;
							return true;
						case -4:
							*kernel_function = &revStride4Write_Word128;
							*dummy_kernel_function = &dummy_revStride4Loop_Word128;
							return true;
						case 8:
							*kernel_function = &forwStride8Write_Word128;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word128;
							return true;
						case -8:
							*kernel_function = &revStride8Write_Word128;
							*dummy_kernel_function = &dummy_revStride8Loop_Word128;
							return true;
						case 16:
							*kernel_function = &forwStride16Write_Word128;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word128;
							return true;
						case -16:
							*kernel_function = &revStride16Write_Word128;
							*dummy_kernel_function = &dummy_revStride16Loop_Word128;
							return true;
						default:
							return false;
					}
					return true;
#endif
#ifdef HAS_WORD_256
				case CHUNK_256b:
					switch (stride_size) {
						case 1:
							*kernel_function = &forwSequentialWrite_Word256;
							*dummy_kernel_function = &dummy_forwSequentialLoop_Word256;
							return true;
						case -1:
							*kernel_function = &revSequentialWrite_Word256;
							*dummy_kernel_function = &dummy_revSequentialLoop_Word256;
							return true;
						case 2:
							*kernel_function = &forwStride2Write_Word256;
							*dummy_kernel_function = &dummy_forwStride2Loop_Word256;
							return true;
						case -2:
							*kernel_function = &revStride2Write_Word256;
							*dummy_kernel_function = &dummy_revStride2Loop_Word256;
							return true;
						case 4:
							*kernel_function = &forwStride4Write_Word256;
							*dummy_kernel_function = &dummy_forwStride4Loop_Word256;
							return true;
						case -4:
							*kernel_function = &revStride4Write_Word256;
							*dummy_kernel_function = &dummy_revStride4Loop_Word256;
							return true;
						case 8:
							*kernel_function = &forwStride8Write_Word256;
							*dummy_kernel_function = &dummy_forwStride8Loop_Word256;
							return true;
						case -8:
							*kernel_function = &revStride8Write_Word256;
							*dummy_kernel_function = &dummy_revStride8Loop_Word256;
							return true;
						case 16:
							*kernel_function = &forwStride16Write_Word256;
							*dummy_kernel_function = &dummy_forwStride16Loop_Word256;
							return true;
						case -16:
							*kernel_function = &revStride16Write_Word256;
							*dummy_kernel_function = &dummy_revStride16Loop_Word256;
							return true;
						default:
							return false;
					}
					return true;
#endif

				default:
					return false;
			}
			return true;

		default:
			return false;
	}

	return false; //shouldn't reach this point
}
	
bool xmem::determineRandomKernel(rw_mode_t rw_mode, chunk_size_t chunk_size, RandomFunction* kernel_function, RandomFunction* dummy_kernel_function) {
	switch (rw_mode) {
		case READ:
			switch (chunk_size) {
				//special case on 32-bit architectures only.
#ifndef HAS_WORD_64
				case CHUNK_32b:
					*kernel_function = &randomRead_Word32;
					*dummy_kernel_function = &dummy_randomLoop_Word32;
					return true;
#endif
#ifdef HAS_WORD_64
				case CHUNK_64b:
					*kernel_function = &randomRead_Word64;
					*dummy_kernel_function = &dummy_randomLoop_Word64;
					return true;
#endif
#ifdef HAS_WORD_128
				case CHUNK_128b:
					*kernel_function = &randomRead_Word128;
					*dummy_kernel_function = &dummy_randomLoop_Word128;
					return true;
#endif
#ifdef HAS_WORD_256
				case CHUNK_256b:
					*kernel_function = &randomRead_Word256;
					*dummy_kernel_function = &dummy_randomLoop_Word256;
					return true;
#endif
				default:
					return false;
			}
			return true;

		case WRITE:
			switch (chunk_size) {
				//special case on 32-bit architectures only.
#ifndef HAS_WORD_64
				case CHUNK_32b:
					*kernel_function = &randomWrite_Word32;
					*dummy_kernel_function = &dummy_randomLoop_Word32;
					return true;
#endif
#ifdef HAS_WORD_64
				case CHUNK_64b:
					*kernel_function = &randomWrite_Word64;
					*dummy_kernel_function = &dummy_randomLoop_Word64;
					return true;
#endif
#ifdef HAS_WORD_128
				case CHUNK_128b:
					*kernel_function = &randomWrite_Word128;
					*dummy_kernel_function = &dummy_randomLoop_Word128;
					return true;
#endif
#ifdef HAS_WORD_256
				case CHUNK_256b:
					*kernel_function = &randomWrite_Word256;
					*dummy_kernel_function = &dummy_randomLoop_Word256;
					return true;
#endif
				default:
					return false;
			}
			return true;

		default:
			return false;
	}

	return false;
}

bool xmem::buildRandomPointerPermutation(void* start_address, void* end_address, chunk_size_t chunk_size) {
	if (g_verbose)
		std::cout << "Preparing a memory region under test. This might take a while...";

	size_t length = reinterpret_cast<uint8_t*>(end_address) - reinterpret_cast<uint8_t*>(start_address); //length of region in bytes
	size_t num_pointers = 0; //Number of pointers that fit into the memory region of interest
	switch (chunk_size) {
		//special case on 32-bit architectures only.
#ifndef HAS_WORD_64
		case CHUNK_32b:
			num_pointers = length / sizeof(Word32_t);
			break;
#endif
#ifdef HAS_WORD_64
		case CHUNK_64b:
			num_pointers = length / sizeof(Word64_t);
			break;
#endif
#ifdef HAS_WORD_128
		case CHUNK_128b:
			num_pointers = length / sizeof(Word128_t);
			break;
#endif
#ifdef HAS_WORD_256
		case CHUNK_256b:
			num_pointers = length / sizeof(Word256_t);
			break;
#endif
		default:
			std::cerr << "ERROR: Chunk size must be at least "
			//special case for 32-bit architectures
#ifndef HAS_WORD_64
			<<"32"
#endif
#ifdef HAS_WORD_64
			<<"64"
#endif
			<< "bits for building a random pointer permutation. This should not have happened." << std::endl;
			return false;
	}
			
	std::mt19937_64 gen(time(NULL)); //Mersenne Twister random number generator, seeded at current time
	
	/*
	//TODO: probably remove this.
	//Build a random directed Hamiltonian Cycle across the memory region 

	//Let W be the list of memory locations that have not been reached yet. Each entry is an index in mem_base.
	std::vector<size_t> W;
	size_t w_index = 0;

	//Initialize W to contain all memory locations, where each memory location appears exactly once in the list. The order does not strictly matter.
	W.resize(num_pointers);
	for (w_index = 0; w_index < num_pointers; w_index++) {
		W.at(w_index) = w_index;
	}
	
	//Build the directed Hamiltonian Cycle
	size_t v = 0; //the current memory location. Always start at the first location for the Hamiltonian Cycle construction
	size_t w = 0; //the next memory location
	w_index = 0;
	while (W.size() > 0) { //while we have not reached all memory locations
		W.erase(W.begin() + w_index);

		//Normal case
		if (W.size() > 0) {
			//Choose the next w_index at random from W
			w_index = gen() % W.size();

			//Extract the memory location corresponding to this w_index
			w = W[w_index];
		} else { //Last element visited needs to point at head of memory to complete the cycle
			w = 0;
		}

		//Create pointer v --> w. This corresponds to a directed edge in the graph with nodes v and w.
		mem_region_base[v] = reinterpret_cast<uintptr_t>(mem_region_base + w);

		//Chase this pointer to move to next step
		v = w;
	}
	*/
	
	//Do a random shuffle of memory pointers
#ifndef HAS_WORD_64 //special case for 32-bit architectures
	Word32_t* mem_region_base = reinterpret_cast<Word32_t*>(start_address);
#endif
#ifdef HAS_WORD_64
	Word64_t* mem_region_base = reinterpret_cast<Word64_t*>(start_address);
#endif
	switch (chunk_size) {
		//special case for 32-bit architectures
#ifndef HAS_WORD_64
		case CHUNK_32b:
			for (size_t i = 0; i < num_pointers; i++) { //Initialize pointers to point at themselves (identity mapping)
				mem_region_base[i] = reinterpret_cast<Word32_t>(mem_region_base+i);
			}
			std::shuffle(mem_region_base, mem_region_base + num_pointers, gen);
			break;
#endif
#ifdef HAS_WORD_64
		case CHUNK_64b:
			for (size_t i = 0; i < num_pointers; i++) { //Initialize pointers to point at themselves (identity mapping)
				mem_region_base[i] = reinterpret_cast<Word64_t>(mem_region_base+i);
			}
			std::shuffle(mem_region_base, mem_region_base + num_pointers, gen);
			break;
#endif
#ifdef HAS_WORD_128
		case CHUNK_128b:
			for (size_t i = 0; i < num_pointers; i++) { //Initialize pointers to point at themselves (identity mapping)
#ifndef HAS_WORD_64 //special case for 32-bit architectures
				mem_region_base[i*4] = reinterpret_cast<Word32_t>(mem_region_base+(i*4));
				mem_region_base[(i*4)+1] = 0xFFFFFFFF; //1-fill upper 96 bits
				mem_region_base[(i*4)+2] = 0xFFFFFFFF; 
				mem_region_base[(i*4)+3] = 0xFFFFFFFF; 
#endif
#ifdef HAS_WORD_64
				mem_region_base[i*2] = reinterpret_cast<Word64_t>(mem_region_base+(i*2));
				mem_region_base[(i*2)+1] = 0xFFFFFFFFFFFFFFFF; //1-fill upper 64 bits
#endif
			}
			std::shuffle(reinterpret_cast<Word128_t*>(mem_region_base), reinterpret_cast<Word128_t*>(mem_region_base) + num_pointers, gen);
			break;
#endif
#ifdef HAS_WORD_256
		case CHUNK_256b:
			for (size_t i = 0; i < num_pointers; i++) { //Initialize pointers to point at themselves (identity mapping)
#ifndef HAS_WORD_64 //special case for 32-bit architectures
				mem_region_base[i*8] = reinterpret_cast<Word32_t>(mem_region_base+(i*8));
				mem_region_base[(i*8)+1] = 0xFFFFFFFF; //1-fill upper 224 bits
				mem_region_base[(i*8)+2] = 0xFFFFFFFF;
				mem_region_base[(i*8)+3] = 0xFFFFFFFF;
				mem_region_base[(i*8)+4] = 0xFFFFFFFF;
				mem_region_base[(i*8)+5] = 0xFFFFFFFF;
				mem_region_base[(i*8)+6] = 0xFFFFFFFF;
				mem_region_base[(i*8)+7] = 0xFFFFFFFF;
#endif
#ifdef HAS_WORD_64
				mem_region_base[i*4] = reinterpret_cast<Word64_t>(mem_region_base+(i*4));
				mem_region_base[(i*4)+1] = 0xFFFFFFFFFFFFFFFF; //1-fill upper 192 bits
				mem_region_base[(i*4)+2] = 0xFFFFFFFFFFFFFFFF; 
				mem_region_base[(i*4)+3] = 0xFFFFFFFFFFFFFFFF;
#endif
			}
			std::shuffle(reinterpret_cast<Word256_t*>(mem_region_base), reinterpret_cast<Word256_t*>(mem_region_base) + num_pointers, gen);
			break;
#endif
		default:
			std::cerr << "ERROR: Got an invalid chunk size. This should not have happened." << std::endl;
			return false;
	}

	if (g_verbose) {
		std::cout << "done" << std::endl;
		std::cout << std::endl;
	}

	return true;
}

/***********************************************************************
 ***********************************************************************
 ********************** LATENCY-RELATED BENCHMARK KERNELS **************
 ***********************************************************************
 ***********************************************************************/

/* --------------------- DUMMY BENCHMARK ROUTINES --------------------------- */

int32_t xmem::dummy_chasePointers(uintptr_t*, uintptr_t**, size_t len) {
	volatile uintptr_t placeholder = 0; //Try to defeat compiler optimizations removing this method
#ifdef USE_SIZE_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case for 32-bit architectures
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 1024)
#endif
#ifdef HAS_WORD_64
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 512)
#endif
		placeholder = 0;
#endif
	return 0;
}

/* -------------------- CORE BENCHMARK ROUTINES -------------------------- */

int32_t xmem::chasePointers(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
	volatile uintptr_t* p = first_address;

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case for 32-bit architectures
	UNROLL1024(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
#ifdef HAS_WORD_64
	UNROLL512(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case for 32-bit architectures
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 1024) {
		UNROLL1024(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
#ifdef HAS_WORD_64
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 512) {
		UNROLL512(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
	}
#endif
	*last_touched_address = const_cast<uintptr_t*>(p);
	return 0;
}


/***********************************************************************
 ***********************************************************************
 ******************* THROUGHPUT-RELATED BENCHMARK KERNELS **************
 ***********************************************************************
 ***********************************************************************/

#if defined(_WIN32) && defined(ARCH_INTEL)
//TODO: will we need ARM asm on Win?

//Hand-coded assembly functions for the SSE2/AVX benchmark routines.
//VC++ compiler does not support inline assembly in x86-64.
//And the compiler optimizes away the vector instructions unless I use volatile.
//But I can't use for example volatile Word256_t* because it is incompatible with _mm_load_si256() with VC++. 
//Fortunately, I implemented the routine as a wrapper around a hand-coded assembler C function.

#ifdef HAS_WORD_128
//128-bit
extern "C" int win_asm_forwSequentialRead_Word128(Word128_t* first_word, Word128_t* last_word);
extern "C" int win_asm_revSequentialRead_Word128(Word128_t* last_word, Word128_t* first_word);
extern "C" int win_asm_forwSequentialWrite_Word128(Word128_t* first_word, Word128_t* last_word);
extern "C" int win_asm_revSequentialWrite_Word128(Word128_t* last_word, Word128_t* first_word); 
#endif

#ifdef HAS_WORD_256
//256-bit
extern "C" int win_asm_forwSequentialRead_Word256(Word256_t* first_word, Word256_t* last_word);
extern "C" int win_asm_revSequentialRead_Word256(Word256_t* last_word, Word256_t* first_word);
extern "C" int win_asm_forwSequentialWrite_Word256(Word256_t* first_word, Word256_t* last_word);
extern "C" int win_asm_revSequentialWrite_Word256(Word256_t* last_word, Word256_t* first_word);
#endif

//Dummies
#ifdef HAS_WORD_128
//128-bit
extern "C" int win_asm_dummy_forwSequentialLoop_Word128(Word128_t* first_word, Word128_t* last_word);
extern "C" int win_asm_dummy_revSequentialLoop_Word128(Word128_t* first_word, Word128_t* last_word); 
#endif

#ifdef HAS_WORD_256
//256-bit
extern "C" int win_asm_dummy_forwSequentialLoop_Word256(Word256_t* first_word, Word256_t* last_word);
extern "C" int win_asm_dummy_revSequentialLoop_Word256(Word256_t* first_word, Word256_t* last_word);
#endif
#endif

/* --------------------- DUMMY BENCHMARK ROUTINES --------------------------- */

int32_t xmem::dummy_empty(void*, void*) { 
	return 0;
}
		
/* ------------ SEQUENTIAL LOOP --------------*/

int32_t xmem::dummy_forwSequentialLoop_Word32(void* start_address, void* end_address) {
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address), *endptr = static_cast<Word32_t*>(end_address); wordptr < endptr;) {
		UNROLL1024(wordptr++;) 
		placeholder = 0;
	}
	return placeholder;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_forwSequentialLoop_Word64(void* start_address, void* end_address) {
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address), *endptr = static_cast<Word64_t*>(end_address); wordptr < endptr;) {
		UNROLL512(wordptr++;) 
		placeholder = 0;
	}
	return placeholder;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_forwSequentialLoop_Word128(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_dummy_forwSequentialLoop_Word128(static_cast<Word128_t*>(start_address), static_cast<Word128_t*>(end_address));
#endif
#ifdef __gnu_linux__
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address), *endptr = static_cast<Word128_t*>(end_address); wordptr < endptr;) {
		UNROLL256(wordptr++;) 
		placeholder = 0;
	}
	return placeholder;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_forwSequentialLoop_Word256(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_dummy_forwSequentialLoop_Word256(static_cast<Word256_t*>(start_address), static_cast<Word256_t*>(end_address));
#endif
#ifdef __gnu_linux__
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address), *endptr = static_cast<Word256_t*>(end_address); wordptr < endptr;) {
		UNROLL128(wordptr++;) 
		placeholder = 0;
	}
	return placeholder;
#endif
}
#endif

int32_t xmem::dummy_revSequentialLoop_Word32(void* start_address, void* end_address) { 
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address), *begptr = static_cast<Word32_t*>(start_address); wordptr > begptr;) {
		UNROLL1024(wordptr--;)
		placeholder = 0;
	}
	return placeholder;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_revSequentialLoop_Word64(void* start_address, void* end_address) {
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address), *begptr = static_cast<Word64_t*>(start_address); wordptr > begptr;) {
		UNROLL512(wordptr--;)
		placeholder = 0;
	}
	return placeholder;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_revSequentialLoop_Word128(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_dummy_revSequentialLoop_Word128(static_cast<Word128_t*>(end_address), static_cast<Word128_t*>(start_address));
#endif
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address), *begptr = static_cast<Word128_t*>(start_address); wordptr > begptr;) {
		UNROLL256(wordptr--;) 
		placeholder = 0;
	}
	return placeholder;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_revSequentialLoop_Word256(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_dummy_revSequentialLoop_Word256(static_cast<Word256_t*>(end_address), static_cast<Word256_t*>(start_address));
#endif
#ifdef __gnu_linux__
	volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address), *begptr = static_cast<Word256_t*>(start_address); wordptr > begptr;) {
		UNROLL128(wordptr--;) 
		placeholder = 0;
	}
	return placeholder;
#endif
}
#endif
		
/* ------------ STRIDE 2 LOOP --------------*/

int32_t xmem::dummy_forwStride2Loop_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 512) {
		UNROLL512(wordptr += 2;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_forwStride2Loop_Word64(void* start_address, void* end_address) {
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 256) {
		UNROLL256(wordptr += 2;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_forwStride2Loop_Word128(void* start_address, void* end_address) {
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 128) {
		UNROLL128(wordptr += 2;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_forwStride2Loop_Word256(void* start_address, void* end_address) {
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 64) {
		UNROLL64(wordptr += 2;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

int32_t xmem::dummy_revStride2Loop_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 512) {
		UNROLL512(wordptr -= 2;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_revStride2Loop_Word64(void* start_address, void* end_address) {
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 256) {
		UNROLL256(wordptr -= 2;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_revStride2Loop_Word128(void* start_address, void* end_address) {
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 128) {
		UNROLL128(wordptr -= 2;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_revStride2Loop_Word256(void* start_address, void* end_address) { 
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 64) {
		UNROLL64(wordptr -= 2;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

/* ------------ STRIDE 4 LOOP --------------*/

int32_t xmem::dummy_forwStride4Loop_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 256) {
		UNROLL256(wordptr += 4;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_forwStride4Loop_Word64(void* start_address, void* end_address) {
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 128) {
		UNROLL128(wordptr += 4;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_forwStride4Loop_Word128(void* start_address, void* end_address) {
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 64) {
		UNROLL64(wordptr += 4;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_forwStride4Loop_Word256(void* start_address, void* end_address) {
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 32) {
		UNROLL32(wordptr += 4;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

int32_t xmem::dummy_revStride4Loop_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 256) {
		UNROLL256(wordptr -= 4;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_revStride4Loop_Word64(void* start_address, void* end_address) {
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 128) {
		UNROLL128(wordptr -= 4;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_revStride4Loop_Word128(void* start_address, void* end_address) {
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 64) {
		UNROLL64(wordptr -= 4;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_revStride4Loop_Word256(void* start_address, void* end_address) {
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 32) {
		UNROLL32(wordptr -= 4;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

/* ------------ STRIDE 8 LOOP --------------*/

int32_t xmem::dummy_forwStride8Loop_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 128) {
		UNROLL128(wordptr += 8;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_forwStride8Loop_Word64(void* start_address, void* end_address) {
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 64) {
		UNROLL64(wordptr += 8;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_forwStride8Loop_Word128(void* start_address, void* end_address) {
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 32) {
		UNROLL32(wordptr += 8;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_forwStride8Loop_Word256(void* start_address, void* end_address) {
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 16) {
		UNROLL16(wordptr += 8;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

int32_t xmem::dummy_revStride8Loop_Word32(void* start_address, void* end_address) { 
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 128) {
		UNROLL128(wordptr -= 8;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_revStride8Loop_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 64) {
		UNROLL64(wordptr -= 8;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_revStride8Loop_Word128(void* start_address, void* end_address) { 
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 32) {
		UNROLL32(wordptr -= 8;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_revStride8Loop_Word256(void* start_address, void* end_address) { 
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 16) {
		UNROLL16(wordptr -= 8;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

/* ------------ STRIDE 16 LOOP --------------*/

int32_t xmem::dummy_forwStride16Loop_Word32(void* start_address, void* end_address) { 
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 64) {
		UNROLL64(wordptr += 16;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_forwStride16Loop_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 32) {
		UNROLL32(wordptr += 16;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_forwStride16Loop_Word128(void* start_address, void* end_address) { 
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 16) {
		UNROLL16(wordptr += 16;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_forwStride16Loop_Word256(void* start_address, void* end_address) { 
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 32) {
		UNROLL8(wordptr += 16;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

int32_t xmem::dummy_revStride16Loop_Word32(void* start_address, void* end_address) { 
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 64) {
		UNROLL64(wordptr -= 16;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::dummy_revStride16Loop_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 32) {
		UNROLL32(wordptr -= 16;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_revStride16Loop_Word128(void* start_address, void* end_address) { 
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 16) {
		UNROLL16(wordptr -= 16;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_revStride16Loop_Word256(void* start_address, void* end_address) { 
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 8) {
		UNROLL8(wordptr -= 16;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

/* ------------ RANDOM LOOP --------------*/

#ifndef HAS_WORD_64 //special case: 32-bit architectures
int32_t xmem::dummy_randomLoop_Word32(uintptr_t*, uintptr_t**, size_t len) {
	volatile uintptr_t* placeholder = NULL; //Try to defeat compiler optimizations removing this method

#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 1024)
		placeholder = NULL;
#endif
	
	return 0;
}
#endif

#ifdef HAS_WORD_64
int32_t xmem::dummy_randomLoop_Word64(uintptr_t*, uintptr_t**, size_t len) {
	volatile uintptr_t* placeholder = NULL; //Try to defeat compiler optimizations removing this method

#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 512)
		placeholder = NULL;
#endif
	
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::dummy_randomLoop_Word128(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	//TODO: check that the compiler generates this code correctly
	volatile Word128_t* placeholder = reinterpret_cast<Word128_t*>(first_address);
	register Word128_t val;
	volatile uintptr_t val_extract;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machines
	UNROLL256(val_extract = my_32b_extractLSB_128b(val);) //Extract 32 LSB.
#endif
#ifdef HAS_WORD_64
	UNROLL256(val_extract = my_64b_extractLSB_128b(val);) //Extract 64 LSB.
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(Word128_t); i += 256) {
#ifndef HAS_WORD_64 //special case: 32-bit machines
		UNROLL256(val_extract = my_32b_extractLSB_128b(val);) //Extract 32 LSB.
#endif
#ifdef HAS_WORD_64
		UNROLL256(val_extract = my_64b_extractLSB_128b(val);) //Extract 64 LSB.
#endif
	}
#endif

	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::dummy_randomLoop_Word256(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	//TODO: check that the compiler generates this code correctly
	volatile Word256_t* placeholder = reinterpret_cast<Word256_t*>(first_address);
	register Word256_t val;
	volatile uintptr_t val_extract;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machines
		UNROLL128(val_extract = my_32b_extractLSB_256b(val);) //Extract 32 LSB.
#endif
#ifdef HAS_WORD_64
		UNROLL128(val_extract = my_64b_extractLSB_256b(val);) //Extract 64 LSB.
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(Word256_t); i += 128) {
#ifndef HAS_WORD_64 //special case: 32-bit machines
		UNROLL128(val_extract = my_32b_extractLSB_256b(val);) //Extract 32 LSB.
#endif
#ifdef HAS_WORD_64
		UNROLL128(val_extract = my_64b_extractLSB_256b(val);) //Extract 64 LSB.
#endif
	}
#endif

	return 0;
#endif
}
#endif

/* -------------------- CORE BENCHMARK ROUTINES -------------------------- 
 *
 * These routines access the memory in different ways for each benchmark type.
 * Optimization tricks include:
 *   - register keyword to hint to compiler values that should not be used in memory access
 *   - UNROLL macros to manual loop unrolling. This reduces the relative branch overhead of the loop.
 *     We don't want to benchmark loops, we want to benchmark memory!
 *   - volatile keyword to prevent compiler from optimizing the code and removing instructions that we need.
 *     The compiler is too smart for its own good!
 *	 - Hardcoding stride and chunk sizes in each benchmark snippet, so that way they do not have to waste cycles being computed at runtime.
 *	   This makes code harder to maintain and to read but ensures more accurate memory benchmarking.
 *
 * ----------------------------------------------------------------------- */

/* ------------ SEQUENTIAL READ --------------*/

int32_t xmem::forwSequentialRead_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address), *endptr = static_cast<Word32_t*>(end_address); wordptr < endptr;) {
		UNROLL1024(val = *wordptr++;) 
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwSequentialRead_Word64(void* start_address, void* end_address) {
	register Word64_t val;
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address), *endptr = static_cast<Word64_t*>(end_address); wordptr < endptr;) {
		UNROLL512(val = *wordptr++;)
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwSequentialRead_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return win_asm_forwSequentialRead_Word128(static_cast<Word128_t*>(start_address), static_cast<Word128_t*>(end_address));
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address), *endptr = static_cast<Word128_t*>(end_address); wordptr < endptr;) {
		UNROLL256(val = *wordptr++;)
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwSequentialRead_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return win_asm_forwSequentialRead_Word256(static_cast<Word256_t*>(start_address), static_cast<Word256_t*>(end_address));
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address), *endptr = static_cast<Word256_t*>(end_address); wordptr < endptr;) {
		UNROLL128(val = *wordptr++;)
	}
	return 0;
#endif
}
#endif

int32_t xmem::revSequentialRead_Word32(void* start_address, void* end_address) { 
	register Word32_t val;
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address), *begptr = static_cast<Word32_t*>(start_address); wordptr > begptr;) {
		UNROLL1024(val = *wordptr--;)
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revSequentialRead_Word64(void* start_address, void* end_address) {
	register Word64_t val;
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address), *begptr = static_cast<Word64_t*>(start_address); wordptr > begptr;) {
		UNROLL512(val = *wordptr--;)
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revSequentialRead_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return win_asm_revSequentialRead_Word128(static_cast<Word128_t*>(end_address), static_cast<Word128_t*>(start_address));
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address), *begptr = static_cast<Word128_t*>(start_address); wordptr > begptr;) {
		UNROLL256(val = *wordptr--;)
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revSequentialRead_Word256(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_revSequentialRead_Word256(static_cast<Word256_t*>(end_address), static_cast<Word256_t*>(start_address));
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address), *begptr = static_cast<Word256_t*>(start_address); wordptr > begptr;) {
		UNROLL128(val = *wordptr--;)
	}
	return 0;
#endif
}
#endif

/* ------------ SEQUENTIAL WRITE --------------*/

int32_t xmem::forwSequentialWrite_Word32(void* start_address, void* end_address) {
	register Word32_t val = 0xFFFFFFFF; 
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address), *endptr = static_cast<Word32_t*>(end_address); wordptr < endptr;) {
		UNROLL1024(*wordptr++ = val;) 
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwSequentialWrite_Word64(void* start_address, void* end_address) {
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address), *endptr = static_cast<Word64_t*>(end_address); wordptr < endptr;) {
		UNROLL512(*wordptr++ = val;) 
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwSequentialWrite_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return win_asm_forwSequentialWrite_Word128(static_cast<Word128_t*>(start_address), static_cast<Word128_t*>(end_address));
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address), *endptr = static_cast<Word128_t*>(end_address); wordptr < endptr;) {
		UNROLL256(*wordptr++ = val;) 
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwSequentialWrite_Word256(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_forwSequentialWrite_Word256(static_cast<Word256_t*>(start_address), static_cast<Word256_t*>(end_address));
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address), *endptr = static_cast<Word256_t*>(end_address); wordptr < endptr;) {
		UNROLL128(*wordptr++ = val;) 
	}
	return 0;
#endif
}
#endif

int32_t xmem::revSequentialWrite_Word32(void* start_address, void* end_address) {
	register Word32_t val = 0xFFFFFFFF;
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address), *begptr = static_cast<Word32_t*>(start_address); wordptr > begptr;) {
		UNROLL1024(*wordptr-- = val;)
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revSequentialWrite_Word64(void* start_address, void* end_address) {
	register Word64_t val = 0xFFFFFFFFFFFFFFFF;
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address), *begptr = static_cast<Word64_t*>(start_address); wordptr > begptr;) {
		UNROLL512(*wordptr-- = val;)
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revSequentialWrite_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return win_asm_revSequentialWrite_Word128(static_cast<Word128_t*>(end_address), static_cast<Word128_t*>(start_address));
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address), *begptr = static_cast<Word128_t*>(start_address); wordptr > begptr;) {
		UNROLL256(*wordptr-- = val;)
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revSequentialWrite_Word256(void* start_address, void* end_address) {
#ifdef _WIN32
	return win_asm_revSequentialWrite_Word256(static_cast<Word256_t*>(end_address), static_cast<Word256_t*>(start_address));
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address), *begptr = static_cast<Word256_t*>(start_address); wordptr > begptr;) {
		UNROLL128(*wordptr-- = val;)
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 2 READ --------------*/

int32_t xmem::forwStride2Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 512) {
		UNROLL512(val = *wordptr; wordptr += 2;) 
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride2Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 256) {
		UNROLL256(val = *wordptr; wordptr += 2;) 
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride2Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 128) {
		UNROLL128(val = *wordptr; wordptr += 2;) 
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride2Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr += 2;) 
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride2Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 512) {
		UNROLL512(val = *wordptr; wordptr -= 2;) 
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride2Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 256) {
		UNROLL256(val = *wordptr; wordptr -= 2;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride2Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = (static_cast<uint32_t>reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 128) {
		UNROLL128(val = *wordptr; wordptr -= 2;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride2Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr -= 2;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 2 WRITE --------------*/

int32_t xmem::forwStride2Write_Word32(void* start_address, void* end_address) {
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 512) {
		UNROLL512(*wordptr = val; wordptr += 2;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride2Write_Word64(void* start_address, void* end_address) {
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 256) {
		UNROLL256(*wordptr = val; wordptr += 2;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride2Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 128) {
		UNROLL128(*wordptr = val; wordptr += 2;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride2Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr += 2;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride2Write_Word32(void* start_address, void* end_address) { 
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 512) {
		UNROLL512(*wordptr = val; wordptr -= 2;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride2Write_Word64(void* start_address, void* end_address) { 
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 256) {
		UNROLL256(*wordptr = val; wordptr -= 2;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride2Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 128) {
		UNROLL128(*wordptr = val; wordptr -= 2;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride2Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr -= 2;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 4 READ --------------*/

int32_t xmem::forwStride4Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 256) {
		UNROLL256(val = *wordptr; wordptr += 4;) 
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride4Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 128) {
		UNROLL128(val = *wordptr; wordptr += 4;) 
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride4Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr += 4;) 
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride4Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 32) {
		UNROLL32(val = *wordptr; wordptr += 4;) 
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride4Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 256) {
		UNROLL256(val = *wordptr; wordptr -= 4;) 
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride4Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 128) {
		UNROLL128(val = *wordptr; wordptr -= 4;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride4Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr -= 4;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride4Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 32) {
		UNROLL32(val = *wordptr; wordptr -= 4;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 4 WRITE --------------*/

int32_t xmem::forwStride4Write_Word32(void* start_address, void* end_address) {
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 256) {
		UNROLL256(*wordptr = val; wordptr += 4;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride4Write_Word64(void* start_address, void* end_address) {
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 128) {
		UNROLL128(*wordptr = val; wordptr += 4;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride4Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr += 4;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride4Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 32) {
		UNROLL32(*wordptr = val; wordptr += 4;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride4Write_Word32(void* start_address, void* end_address) { 
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 256) {
		UNROLL256(*wordptr = val; wordptr -= 4;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride4Write_Word64(void* start_address, void* end_address) { 
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 128) {
		UNROLL128(*wordptr = val; wordptr -= 4;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride4Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr -= 4;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride4Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 32) {
		UNROLL32(*wordptr = val; wordptr -= 4;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 8 READ --------------*/

int32_t xmem::forwStride8Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 128) {
		UNROLL128(val = *wordptr; wordptr += 8;) 
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride8Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr += 8;) 
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride8Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 32) {
		UNROLL32(val = *wordptr; wordptr += 8;) 
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride8Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 16) {
		UNROLL16(val = *wordptr; wordptr += 8;) 
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride8Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 128) {
		UNROLL128(val = *wordptr; wordptr -= 8;) 
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride8Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr -= 8;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride8Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 32) {
		UNROLL32(val = *wordptr; wordptr -= 8;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride8Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 16) {
		UNROLL16(val = *wordptr; wordptr -= 8;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 8 WRITE --------------*/

int32_t xmem::forwStride8Write_Word32(void* start_address, void* end_address) {
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 128) {
		UNROLL128(*wordptr = val; wordptr += 8;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride8Write_Word64(void* start_address, void* end_address) {
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr += 8;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride8Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 32) {
		UNROLL32(*wordptr = val; wordptr += 8;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride8Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 16) {
		UNROLL16(*wordptr = val; wordptr += 8;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride8Write_Word32(void* start_address, void* end_address) { 
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 128) {
		UNROLL128(*wordptr = val; wordptr -= 8;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride8Write_Word64(void* start_address, void* end_address) { 
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr -= 8;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride8Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 32) {
		UNROLL32(*wordptr = val; wordptr -= 8;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride8Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 16) {
		UNROLL16(*wordptr = val; wordptr -= 8;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 16 READ --------------*/

int32_t xmem::forwStride16Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr += 16;) 
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride16Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 32) {
		UNROLL32(val = *wordptr; wordptr += 16;) 
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride16Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 16) {
		UNROLL16(val = *wordptr; wordptr += 16;) 
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride16Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 8) {
		UNROLL8(val = *wordptr; wordptr += 16;) 
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride16Read_Word32(void* start_address, void* end_address) {
	register Word32_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 64) {
		UNROLL64(val = *wordptr; wordptr -= 16;) 
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride16Read_Word64(void* start_address, void* end_address) { 
	register Word64_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 32) {
		UNROLL32(val = *wordptr; wordptr -= 16;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride16Read_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 16) {
		UNROLL16(val = *wordptr; wordptr -= 16;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride16Read_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 8) {
		UNROLL8(val = *wordptr; wordptr -= 16;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}
	return 0;
#endif
}
#endif

/* ------------ STRIDE 16 WRITE --------------*/

int32_t xmem::forwStride16Write_Word32(void* start_address, void* end_address) {
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(start_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr += 16;)
		if (wordptr >= static_cast<Word32_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::forwStride16Write_Word64(void* start_address, void* end_address) {
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(start_address); i < len; i += 32) {
		UNROLL32(*wordptr = val; wordptr += 16;)
		if (wordptr >= static_cast<Word64_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::forwStride16Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0;
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(start_address); i < len; i += 16) {
		UNROLL16(*wordptr = val; wordptr += 16;)
		if (wordptr >= static_cast<Word128_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::forwStride16Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0;
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(start_address); i < len; i += 8) {
		UNROLL8(*wordptr = val; wordptr += 16;)
		if (wordptr >= static_cast<Word256_t*>(end_address)) //end, modulo
			wordptr -= len;
	}
	return 0;
#endif
}
#endif

int32_t xmem::revStride16Write_Word32(void* start_address, void* end_address) { 
	register Word32_t val = 0xFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word32_t);
	for (volatile Word32_t* wordptr = static_cast<Word32_t*>(end_address); i < len; i += 64) {
		UNROLL64(*wordptr = val; wordptr -= 16;)
		if (wordptr <= static_cast<Word32_t*>(start_address)) //end, modulo
			wordptr += len;
	}

	return 0;
}

#ifdef HAS_WORD_64
int32_t xmem::revStride16Write_Word64(void* start_address, void* end_address) { 
	register Word64_t val = 0xFFFFFFFFFFFFFFFF; 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word64_t);
	for (volatile Word64_t* wordptr = static_cast<Word64_t*>(end_address); i < len; i += 32) {
		UNROLL32(*wordptr = val; wordptr -= 16;)
		if (wordptr <= static_cast<Word64_t*>(start_address)) //end, modulo
			wordptr += len;
	}

	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::revStride16Write_Word128(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word128_t val;
	val = my_64b_set_128b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word128_t);
	for (volatile Word128_t* wordptr = static_cast<Word128_t*>(end_address); i < len; i += 16) {
		UNROLL16(*wordptr = val; wordptr -= 16;)
		if (wordptr <= static_cast<Word128_t*>(start_address)) //end, modulo
			wordptr += len;
	}

	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::revStride16Write_Word256(void* start_address, void* end_address) { 
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	register Word256_t val;
	val = my_64b_set_256b_word(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF); 
	register uint32_t i = 0;
	register uint32_t len = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(end_address)-reinterpret_cast<uintptr_t>(start_address)) / sizeof(Word256_t);
	for (volatile Word256_t* wordptr = static_cast<Word256_t*>(end_address); i < len; i += 8) {
		UNROLL8(*wordptr = val; wordptr -= 16;)
		if (wordptr <= static_cast<Word256_t*>(start_address)) //end, modulo
			wordptr += len;
	}

	return 0;
#endif
}
#endif

/* ------------ RANDOM READ --------------*/

#ifdef HAS_WORD_64
int32_t xmem::randomRead_Word64(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
	volatile uintptr_t* p = first_address;

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machine
	UNROLL1024(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
#ifdef HAS_WORD_64
	UNROLL512(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 512) {
#ifndef HAS_WORD_64 //special case: 32-bit machine
		UNROLL1024(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
#ifdef HAS_WORD_64
		UNROLL512(p = reinterpret_cast<uintptr_t*>(*p);)
#endif
	}
#endif
	*last_touched_address = const_cast<uintptr_t*>(p);
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::randomRead_Word128(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	volatile Word128_t* p = reinterpret_cast<Word128_t*>(first_address);
	register Word128_t val;

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machine
	UNROLL256(val = *p; p = reinterpret_cast<Word128_t*>(my_32b_extractLSB_128b(val));) //Do 128-bit load. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
	UNROLL256(val = *p; p = reinterpret_cast<Word128_t*>(my_64b_extractLSB_128b(val));) //Do 128-bit load. Then extract 64 LSB to use as next load address.
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(Word128_t); i += 256) {
#ifndef HAS_WORD_64 //special case: 32-bit machine
		UNROLL256(val = *p; p = reinterpret_cast<Word128_t*>(my_32b_extractLSB_128b(val));) //Do 128-bit load. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
		UNROLL256(val = *p; p = reinterpret_cast<Word128_t*>(my_64b_extractLSB_128b(val));) //Do 128-bit load. Then extract 64 LSB to use as next load address.
#endif
	}
#endif

	*last_touched_address = reinterpret_cast<uintptr_t*>(const_cast<Word128_t*>(p)); //Trick compiler. First get rid of volatile qualifier, and then reinterpret pointer
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::randomRead_Word256(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	volatile Word256_t* p = reinterpret_cast<Word256_t*>(first_address);
	register Word256_t val;

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machine
	UNROLL128(val = *p; p = reinterpret_cast<Word256_t*>(my_32b_extractLSB_256b(val));) //Do 256-bit load. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
	UNROLL128(val = *p; p = reinterpret_cast<Word256_t*>(my_64b_extractLSB_256b(val));) //Do 256-bit load. Then extract 64 LSB to use as next load address.
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(Word256_t); i += 128) {
#ifndef HAS_WORD_64 //special case: 32-bit machine
		UNROLL128(val = *p; p = reinterpret_cast<Word256_t*>(my_32b_extractLSB_256b(val));) //Do 256-bit load. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
		UNROLL128(val = *p; p = reinterpret_cast<Word256_t*>(my_64b_extractLSB_256b(val));) //Do 256-bit load. Then extract 64 LSB to use as next load address.
#endif
	}
#endif

	*last_touched_address = reinterpret_cast<uintptr_t*>(const_cast<Word256_t*>(p)); //Trick compiler. First get rid of volatile qualifier, and then reinterpret pointer
	return 0;
#endif
}
#endif

/* ------------ RANDOM WRITE --------------*/

#ifndef HAS_WORD_64 //special case: 32-bit machine
int32_t xmem::randomWrite_Word32(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
	volatile uintptr_t* p = first_address;
	volatile uintptr_t* p2 = NULL;

#ifdef USE_TIME_BASED_BENCHMARKS
	UNROLL1024(p2 = reinterpret_cast<uintptr_t*>(*p); *p = reinterpret_cast<uintptr_t>(p2); p = p2;)
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 1024) {
		UNROLL1024(p2 = reinterpret_cast<uintptr_t*>(*p); *p = reinterpret_cast<uintptr_t>(p2); p = p2;)
	}
#endif
	*last_touched_address = const_cast<uintptr_t*>(p);
	return 0;
}
#endif

#ifdef HAS_WORD_64
int32_t xmem::randomWrite_Word64(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
	volatile uintptr_t* p = first_address;
	volatile uintptr_t* p2 = NULL;

#ifdef USE_TIME_BASED_BENCHMARKS
	UNROLL512(p2 = reinterpret_cast<uintptr_t*>(*p); *p = reinterpret_cast<uintptr_t>(p2); p = p2;)
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(uintptr_t); i += 512) {
		UNROLL512(p2 = reinterpret_cast<uintptr_t*>(*p); *p = reinterpret_cast<uintptr_t>(p2); p = p2;)
	}
#endif
	*last_touched_address = const_cast<uintptr_t*>(p);
	return 0;
}
#endif

#ifdef HAS_WORD_128
int32_t xmem::randomWrite_Word128(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	volatile Word128_t* p = reinterpret_cast<Word128_t*>(first_address);
	register Word128_t val;

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machine
	UNROLL256(val = *p; *p = val; p = reinterpret_cast<Word128_t*>(my_32b_extractLSB_128b(val));) //Do 128-bit load. Then do 128-bit store. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
	UNROLL256(val = *p; *p = val; p = reinterpret_cast<Word128_t*>(my_64b_extractLSB_128b(val));) //Do 128-bit load. Then do 128-bit store. Then extract 64 LSB to use as next load address.
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(Word128_t); i += 256) {
#ifndef HAS_WORD_64 //special case: 32-bit machine
		UNROLL256(val = *p; *p = val; p = reinterpret_cast<Word128_t*>(my_32b_extractLSB_128b(val));) //Do 128-bit load. Then do 128-bit store. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
		UNROLL256(val = *p; *p = val; p = reinterpret_cast<Word128_t*>(my_64b_extractLSB_128b(val));) //Do 128-bit load. Then do 128-bit store. Then extract 64 LSB to use as next load address.
#endif
	}
#endif

	*last_touched_address = reinterpret_cast<uintptr_t*>(const_cast<Word128_t*>(p)); //Trick compiler. First get rid of volatile qualifier, and then reinterpret pointer
	return 0;
#endif
}
#endif

#ifdef HAS_WORD_256
int32_t xmem::randomWrite_Word256(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
#ifdef _WIN32
	return 0; //TODO: Implement for Windows.
#endif
#ifdef __gnu_linux__
	volatile Word256_t* p = reinterpret_cast<Word256_t*>(first_address);
	register Word256_t val;

#ifdef USE_TIME_BASED_BENCHMARKS
#ifndef HAS_WORD_64 //special case: 32-bit machine
	UNROLL128(val = *p; *p = val; p = reinterpret_cast<Word256_t*>(my_32b_extractLSB_256b(val));) //Do 256-bit load. Then do 256-bit store. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
	UNROLL128(val = *p; *p = val; p = reinterpret_cast<Word256_t*>(my_64b_extractLSB_256b(val));) //Do 256-bit load. Then do 256-bit store. Then extract 64 LSB to use as next load address.
#endif
#endif
#ifdef USE_SIZE_BASED_BENCHMARKS
	for (size_t i = 0; i < len / sizeof(Word256_t); i += 128) {
#ifndef HAS_WORD_64 //special case: 32-bit machine
		UNROLL128(val = *p; *p = val; p = reinterpret_cast<Word256_t*>(my_32b_extractLSB_256b(val));) //Do 256-bit load. Then do 256-bit store. Then extract 32 LSB to use as next load address.
#endif
#ifdef HAS_WORD_64
		UNROLL128(val = *p; *p = val; p = reinterpret_cast<Word256_t*>(my_64b_extractLSB_256b(val));) //Do 256-bit load. Then do 256-bit store. Then extract 64 LSB to use as next load address.
#endif
	}
#endif

	*last_touched_address = reinterpret_cast<uintptr_t*>(const_cast<Word256_t*>(p)); //Trick compiler. First get rid of volatile qualifier, and then reinterpret pointer
	return 0;
#endif
}
#endif
