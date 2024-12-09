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
	size_t current_bytes; ///< Current heap memory usage in bytes
	size_t current_effective_bytes; ///< Current effective memory usage
	size_t new_allocations_counter; ///< Number of new memory allocations
	size_t total_allocated_bytes; ///< Total heap memory allocated
	size_t total_effective_allocated_bytes; ///< Total effective memory allocated
	size_t actual_reallocations_counter; ///< Count of actual reallocations
	size_t realloc_optimized_counter; ///< Count of optimized reallocations
	size_t free_counter; ///< Number of memory deallocations
	size_t free_total_bytes; ///< Total bytes freed
	size_t peak_bytes; ///< Peak heap memory usage
	size_t realloc_noop_counter; ///< Count of no-op reallocations
} Telemetry;

extern Telemetry telemetry;

/**
 * @brief Tracks heap trick reallocation events
 * @details Atomically increments the counter for optimized reallocations that avoid actual memory operations
 */
void telemetry_realloc_optimized_counter(void);

/**
 * @brief Tracks new memory allocation events
 * @details Atomically increments the counter for new memory allocations
 */
void telemetry_allocations_counter(void);

/**
 * @brief Tracks actual memory reallocation events
 * @details Atomically increments the counter for true memory reallocations that modify system memory
 */
void telemetry_actual_reallocations_counter(void);

/**
 * @brief Tracks no-op reallocation events
 * @details Atomically increments the counter when realloc is called but no action is needed
 */
void telemetry_realloc_noop_counter(void);

/**
 * @brief Tracks memory deallocation events
 * @details Atomically increments the counter for memory free operations
 */
void telemetry_free_counter(void);

/**
 * @brief Tracks total bytes freed
 * @param amount_of_bytes Number of bytes being freed
 * @details Atomically adds the freed bytes to the running total
 */
void telemetry_free_total_bytes(const size_t);

/**
 * @brief Updates heap memory allocation metrics
 * @param amount_of_bytes Number of bytes being allocated
 * @details
 * - Updates current heap memory usage
 * - Updates total allocated memory
 * - Updates peak memory usage if current usage exceeds previous peak
 */
void telemetry_add(const size_t);

/**
 * @brief Updates effective memory allocation metrics
 * @param amount_of_bytes Number of bytes being allocated
 * @details
 * - Updates total effective memory allocated
 * - Updates current effective memory usage
 */
void telemetry_effective_add(const size_t amount_of_bytes);

/**
 * @brief Updates heap memory reduction metrics
 * @param amount_of_bytes Number of bytes being reduced
 * @details Atomically subtracts bytes from current heap memory usage
 */
void telemetry_reduce(const size_t amount_of_bytes);

/**
 * @brief Updates effective memory reduction metrics
 * @param amount_of_bytes Number of bytes being reduced
 * @details Atomically subtracts bytes from current effective memory usage
 */
void telemetry_effective_reduce(const size_t amount_of_bytes);

/**
 * @brief Displays telemetry statistics
 * @details Prints memory usage metrics including:
 */
void telemetry_show(void);

/**
 * @brief Initializes telemetry structure
 * @details Zeroes all fields in global telemetry structure.
 * Must be called once before memory operations start.
 */
void init_telemetry(void);
