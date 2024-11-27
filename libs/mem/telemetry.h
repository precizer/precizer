#pragma once

/**
 *
 * @file telemetry.h
 * @brief Function Prototypes and Telemetry Structures
 *
 */

/**
 * @brief Telemetry tracking structure
 * @details Stores memory usage statistics and allocation tracking information
 */
typedef struct {
	size_t current_heap_memory_bytes;   ///< Current heap memory usage in bytes
	size_t current_effective_memory_bytes; ///< Current effective memory usage
	size_t new_allocations_counter;        ///< Number of new memory allocations
	size_t total_heap_memory_allocated_bytes; ///< Total heap memory allocated
	size_t total_effective_memory_allocated_bytes; ///< Total effective memory
	size_t heap_true_reallocations_counter; ///< Actual reallocation count
	size_t heap_trick_reallocations_counter; ///< Optimized reallocation count
	size_t free_counter;            ///< Number of memory deallocations
	size_t free_bytes;                  ///< Total bytes freed
	size_t max_heap_memory_bytes_are_utilized_at_the_same_time_instant; ///< Peak memory usage
	size_t how_many_times_realloc_do_nothing; ///< No-op reallocation count
} Telemetry;

extern Telemetry telemetry;

void telemetry_heap_trick_reallocations_counter(void);
void telemetry_heap_allocations_counter(void);
void telemetry_heap_true_reallocations_counter(void);
void telemetry_how_many_times_realloc_do_nothing(void);
void telemetry_heap_free_counter(void);
void telemetry_heap_free_bytes(
	const size_t
);
void telemetry_heap_add(
	const size_t
);
void telemetry_effective_add(
	const size_t
);
void telemetry_heap_reduce(
	const size_t
);
void telemetry_effective_reduce(
	const size_t
);
void telemetry_show(void);
void init_telemetry(void);
