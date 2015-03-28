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
 * @brief Implementation file for the LatencyBenchmark_Delays class.
 */

//Headers
#include <LatencyBenchmark_Delays.h>
#include <common.h>
#include <benchmark_kernels.h>
#include <MemoryWorker.h>
#include <LatencyWorker.h>
#include <LoadWorker.h>

//Libraries
#include <iostream>
#include <random>
#include <assert.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#endif

using namespace xmem;
		
LatencyBenchmark_Delays::LatencyBenchmark_Delays(
		void* mem_array,
		size_t len,
		uint32_t iterations,
#ifdef USE_SIZE_BASED_BENCHMARKS
		uint64_t passes_per_iteration,
#endif
		uint32_t num_worker_threads,
		uint32_t mem_node,
		uint32_t cpu_node,
		std::vector<PowerReader*> dram_power_readers,
		std::string name
	) :
		LatencyBenchmark(
			mem_array,
			len,
			iterations,
#ifdef USE_SIZE_BASED_BENCHMARKS
			passes_per_iteration,
#endif
			num_worker_threads,
			mem_node,
			cpu_node,
			SEQUENTIAL,
			READ,
			CHUNK_64b,
			1,
			dram_power_readers,
			name
		)
	{ 
}

bool LatencyBenchmark_Delays::_run_core() {
	size_t len_per_thread = _len / _num_worker_threads; //Carve up memory space so each worker has its own area to play in

	//Set up latency measurement kernel function pointers
	RandomFunction lat_kernel_fptr = &chasePointers;
	RandomFunction lat_kernel_dummy_fptr = &dummy_chasePointers;

	//Initialize memory regions for all threads by writing to them, causing the memory to be physically resident.
	forwSequentialWrite_Word64(_mem_array,
							   reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_mem_array)+_len)); //static casts to silence compiler warnings

	//Build pointer indices for random-access latency thread. We assume that latency thread is the first one, so we use beginning of memory region.
	if (!buildRandomPointerPermutation(_mem_array,
									   reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_mem_array)+len_per_thread), //static casts to silence compiler warnings
									   CHUNK_64b)) { 
		std::cerr << "ERROR: Failed to build a random pointer permutation for the latency measurement thread!" << std::endl;
		return false;
	}

	//Set up load generation kernel function pointers
	SequentialFunction load_kernel_fptr_seq = NULL;
	SequentialFunction load_kernel_dummy_fptr_seq = NULL; 
	RandomFunction load_kernel_fptr_ran = NULL;
	RandomFunction load_kernel_dummy_fptr_ran = NULL; 
	if (_num_worker_threads > 1) { //If we only have one worker thread, it is used for latency measurement only, and no load threads will be used.
		if (_pattern_mode == SEQUENTIAL) {
			if (!determineSequentialKernel(_rw_mode, _chunk_size, _stride_size, &load_kernel_fptr_seq, &load_kernel_dummy_fptr_seq)) {
				std::cerr << "ERROR: Failed to find appropriate benchmark kernel." << std::endl;
				return false;
			}
		} else if (_pattern_mode == RANDOM) {
			if (!determineRandomKernel(_rw_mode, _chunk_size, &load_kernel_fptr_ran, &load_kernel_dummy_fptr_ran)) {
				std::cerr << "ERROR: Failed to find appropriate benchmark kernel." << std::endl;
				return false;
			}

			//Build pointer indices for random-access load threads. Note that the pointers for each load thread must stay within its respective region, otherwise sharing may occur. 
			for (uint32_t i = 1; i < _num_worker_threads; i++) {
				if (!buildRandomPointerPermutation(reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_mem_array) + i*len_per_thread), //static casts to silence compiler warnings
												   reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_mem_array) + (i+1)*len_per_thread), //static casts to silence compiler warnings
							 					   _chunk_size)) {
					std::cerr << "ERROR: Failed to build a random pointer permutation for a load generation thread!" << std::endl;
					return false;
				}
			}
		} else {
			std::cerr << "ERROR: Got an invalid pattern mode." << std::endl;
			return false;
		}
	}

	//Set up some stuff for worker threads
	std::vector<MemoryWorker*> workers;
	std::vector<Thread*> worker_threads;
	
	//Start power measurement
	if (g_verbose)
		std::cout << "Starting power measurement threads...";
	
	if (!_start_power_threads()) {
		if (g_verbose)
			std::cout << "FAIL" << std::endl;
		std::cerr << "WARNING: Failed to start power threads." << std::endl;
	} else if (g_verbose)
		std::cout << "done" << std::endl;
	
	//Run benchmark
	if (g_verbose)
		std::cout << "Running benchmark." << std::endl << std::endl;

	//Do a bunch of iterations of the core benchmark routine
	for (uint32_t i = 0; i < _iterations; i++) {

		//Create load workers and load worker threads
		for (uint32_t t = 0; t < _num_worker_threads; t++) {
			void* thread_mem_array = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_mem_array) + t*len_per_thread);
			int32_t cpu_id = cpu_id_in_numa_node(_cpu_node, t);
			if (cpu_id < 0)
				std::cerr << "WARNING: Failed to find logical CPU " << t << " in NUMA node " << _cpu_node << std::endl;
			if (t == 0) { //special case: thread 0 is always latency thread
				workers.push_back(new LatencyWorker(thread_mem_array,
												    len_per_thread,
#ifdef USE_SIZE_BASED_BENCHMARKS
												    _passes_per_iteration,
#endif
												    lat_kernel_fptr,
												    lat_kernel_dummy_fptr,
												    cpu_id));
			} else {
				if (_pattern_mode == SEQUENTIAL)
					workers.push_back(new LoadWorker(thread_mem_array,
													 len_per_thread,
#ifdef USE_SIZE_BASED_BENCHMARKS
													 _passes_per_iteration,
#endif
													 load_kernel_fptr_seq,
													 load_kernel_dummy_fptr_seq,
													 cpu_id));
				else if (_pattern_mode == RANDOM)
					workers.push_back(new LoadWorker(thread_mem_array,
													 len_per_thread,
#ifdef USE_SIZE_BASED_BENCHMARKS
													 _passes_per_iteration,
#endif
													 load_kernel_fptr_ran,
													 load_kernel_dummy_fptr_ran,
													 cpu_id));
				else
					std::cerr << "WARNING: Invalid benchmark pattern mode." << std::endl;
			}
			worker_threads.push_back(new Thread(workers[t]));
		}

		//Start worker threads! gogogo
		for (uint32_t t = 0; t < _num_worker_threads; t++)
			worker_threads[t]->create_and_start();

		//Wait for all threads to complete
		for (uint32_t t = 0; t < _num_worker_threads; t++)
			if (!worker_threads[t]->join())
				std::cerr << "WARNING: A worker thread failed to complete correctly!" << std::endl;
		
		//Compute metrics for this iteration
		bool iter_warning = false;

		//Compute latency metric
		uint64_t lat_passes = workers[0]->getPasses();	
		uint64_t lat_adjusted_ticks = workers[0]->getAdjustedTicks();
		uint64_t lat_elapsed_dummy_ticks = workers[0]->getElapsedDummyTicks();
		uint64_t lat_bytes_per_pass = workers[0]->getBytesPerPass();
		uint64_t lat_accesses_per_pass = lat_bytes_per_pass / 8;
		iter_warning |= workers[0]->hadWarning();
		
		//Compute throughput generated by load threads
		uint64_t load_total_passes = 0;
		uint64_t load_total_adjusted_ticks = 0;
		uint64_t load_total_elapsed_dummy_ticks = 0;
		uint64_t load_bytes_per_pass = 0;
		double load_avg_adjusted_ticks = 0;
		for (uint32_t t = 1; t < _num_worker_threads; t++) {
			load_total_passes += workers[t]->getPasses();
			load_total_adjusted_ticks += workers[t]->getAdjustedTicks();
			load_total_elapsed_dummy_ticks += workers[t]->getElapsedDummyTicks();
			load_bytes_per_pass = workers[t]->getBytesPerPass(); //all should be the same.
			iter_warning |= workers[t]->hadWarning();
		}

		//Compute load metrics for this iteration
		load_avg_adjusted_ticks = static_cast<double>(load_total_adjusted_ticks) / (_num_worker_threads-1);
		if (_num_worker_threads > 1)
			_loadMetricOnIter[i] = (((static_cast<double>(load_total_passes) * static_cast<double>(load_bytes_per_pass)) / static_cast<double>(MB)))   /  ((load_avg_adjusted_ticks * g_ns_per_tick) / 1e9);

		if (iter_warning)
			_warning = true;
	
		if (g_verbose) { //Report metrics for this iteration
			//Latency thread
			std::cout << "Iter " << i+1 << " had " << lat_passes << " latency measurement passes, with " << lat_accesses_per_pass << " accesses per pass:";
			if (iter_warning) std::cout << " -- WARNING";
			std::cout << std::endl;

			std::cout << "...lat clock ticks == " << lat_adjusted_ticks << " (adjusted by -" << lat_elapsed_dummy_ticks << ")";
			if (iter_warning) std::cout << " -- WARNING";
			std::cout << std::endl;

			std::cout << "...lat ns == " << lat_adjusted_ticks * g_ns_per_tick << " (adjusted by -" << lat_elapsed_dummy_ticks * g_ns_per_tick << ")";
			if (iter_warning) std::cout << " -- WARNING";
			std::cout << std::endl;

			std::cout << "...lat sec == " << lat_adjusted_ticks * g_ns_per_tick / 1e9 << " (adjusted by -" << lat_elapsed_dummy_ticks * g_ns_per_tick / 1e9 << ")";
			if (iter_warning) std::cout << " -- WARNING";
			std::cout << std::endl;

			//Load threads
			if (_num_worker_threads > 1) {
				std::cout << "Iter " << i+1 << " had " << load_total_passes << " total load generation passes, with " << load_bytes_per_pass << " bytes per pass:";
				if (iter_warning) std::cout << " -- WARNING";
				std::cout << std::endl;

				std::cout << "...load total clock ticks across " << _num_worker_threads-1 << " threads == " << load_total_adjusted_ticks << " (adjusted by -" << load_total_elapsed_dummy_ticks << ")";
				if (iter_warning) std::cout << " -- WARNING";
				std::cout << std::endl;

				std::cout << "...load total ns across " << _num_worker_threads-1 << " threads == " << load_total_adjusted_ticks * g_ns_per_tick << " (adjusted by -" << load_total_elapsed_dummy_ticks * g_ns_per_tick << ")";
				if (iter_warning) std::cout << " -- WARNING";
				std::cout << std::endl;

				std::cout << "...load total sec across " << _num_worker_threads-1 << " threads == " << load_total_adjusted_ticks * g_ns_per_tick / 1e9 << " (adjusted by -" << load_total_elapsed_dummy_ticks * g_ns_per_tick / 1e9 << ")";
				if (iter_warning) std::cout << " -- WARNING";
				std::cout << std::endl;
			}

		}
		
		//Compute overall metrics for this iteration
		_metricOnIter[i] = static_cast<double>(lat_adjusted_ticks * g_ns_per_tick)  /  static_cast<double>(lat_accesses_per_pass * lat_passes);
		_averageMetric += _metricOnIter[i];
		_averageLoadMetric += _loadMetricOnIter[i];
		
		//Clean up workers and threads for this iteration
		for (uint32_t t = 0; t < _num_worker_threads; t++) {
			delete worker_threads[t];
			delete workers[t];
		}
		worker_threads.clear();
		workers.clear();
	}

	//Stop power measurement
	if (g_verbose) {
		std::cout << std::endl;
		std::cout << "Stopping power measurement threads...";
	}
	
	if (!_stop_power_threads()) {
		if (g_verbose)
			std::cout << "FAIL" << std::endl;
		std::cerr << "WARNING: Failed to stop power measurement threads." << std::endl;
	} else if (g_verbose)
		std::cout << "done" << std::endl;
	
	//Run metadata
	_averageMetric /= static_cast<double>(_iterations);
	_averageLoadMetric /= static_cast<double>(_iterations);
	_hasRun = true;

	return true;
}