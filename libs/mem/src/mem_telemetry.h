#pragma once

#include <stddef.h>

/**
 * @file mem_telemetry.h
 * @brief API for tracking memory helper telemetry counters.
 */

/**
 * @brief Aggregated memory usage statistics collected at runtime.
 */
typedef struct {
	/**
	 * @brief Total bytes currently held by every descriptor in the OS allocator.
	 * @details Includes slab-style block padding. Grows whenever the helper acquires memory
	 *          from the OS and shrinks only when a descriptor releases its buffer via
	 *          @ref mem_delete or a zero-length @ref m_resize request.
	 */
	size_t current_heap_reserved_bytes;
	/**
	 * @brief Logical payload bytes that callers can actually use.
	 * @details Equals `sum(length * single_element_size)` across descriptors and ignores
	 *          slab-style block padding so that business logic can track real data pressure.
	 */
	size_t current_payload_bytes;
	/**
	 * @brief Count of descriptors that grew from zero bytes to a non-zero allocation.
	 * @details Highlights how often brand-new buffers are provisioned (independent of realloc).
	 */
	size_t fresh_heap_allocations;
	/**
	 * @brief Count of calloc-style growth events.
	 * @details Incremented when the helper both allocates fresh memory and zeroes it (e.g., first m_resize after create + calloc semantics).
	 */
	size_t zero_initialized_payload_growths;
	/**
	 * @brief Cumulative bytes ever requested from the OS allocator.
	 * @details Includes slab-style block padding. Useful to estimate the real heap footprint over the lifetime of the process.
	 */
	size_t total_heap_reserved_bytes_acquired;
	/**
	 * @brief Cumulative logical payload bytes that user code asked for.
	 * @details Helps compare requested payload versus actual heap usage (see block-overhead metrics).
	 */
	size_t total_payload_bytes_added;
	/**
	 * @brief Count of reallocations that delegated to the OS allocator.
	 * @details Includes both growth and shrink requests that required `realloc` rather than metadata-only adjustments.
	 */
	size_t heap_reallocations;
	/**
	 * @brief Count of resizes served without touching the OS.
	 * @details Tracks optimized cases where existing capacity was sufficient and only bookkeeping changed.
	 */
	size_t in_place_resizes;
	/**
	 * @brief Number of times buffers were freed back to the operating system.
	 * @details Each successful `mem_delete` contributes exactly once, enabling leak detection.
	 */
	size_t heap_buffer_releases;
	/**
	 * @brief Total bytes returned to the OS.
	 * @details Complements @ref total_heap_reserved_bytes_acquired to show how much memory was recycled.
	 */
	size_t total_heap_reserved_bytes_released;
	/**
	 * @brief Count of shrink operations performed due to @ref RELEASE_UNUSED.
	 * @details Tracks how often callers requested aggressive trimming of buffers.
	 */
	size_t release_unused_shrinks;
	/**
	 * @brief Total bytes returned specifically via @ref RELEASE_UNUSED.
	 * @details Helps quantify the effectiveness of aggressive shrink requests.
	 */
	size_t total_release_unused_heap_reserved_bytes_released;
	/**
	 * @brief Historical maximum of @ref current_heap_reserved_bytes.
	 * @details Indicates the highest simultaneous heap footprint achieved by the helper.
	 */
	size_t peak_heap_reserved_bytes;
	/**
	 * @brief Count of m_resize requests that exactly matched the current logical size.
	 * @details Correlates with redundant `m_resize` calls and feeds the no-op streak metrics.
	 */
	size_t noop_resizes;
	/**
	 * @brief Number of failed `malloc`/`calloc` style calls.
	 * @details Allows operators to differentiate between logic issues and genuine resource exhaustion.
	 */
	size_t heap_allocation_failures;
	/**
	 * @brief Number of failed `realloc` attempts.
	 * @details Highlights pressure scenarios where existing buffers could not grow because the OS refused the request.
	 */
	size_t heap_reallocation_failures;
	/**
	 * @brief Block-overhead bytes currently wasted across all descriptors.
	 * @details Equals `sum(slab_size_bytes - payload_bytes)` and exposes how much memory sits idle due to slab-style block sizing.
	 */
	size_t current_block_overhead_bytes;
	/**
	 * @brief Cumulative block-overhead bytes ever observed.
	 * @details Monotonic counter that grows whenever additional block padding becomes necessary, useful for budgeting internal fragmentation.
	 */
	size_t total_block_overhead_bytes_added;
	/**
	 * @brief Historical maximum of @ref current_block_overhead_bytes.
	 * @details Shows the worst-case block overhead simultaneously held in memory.
	 */
	size_t peak_block_overhead_bytes;
	/**
	 * @brief Current length of the latest consecutive no-op m_resize streak.
	 * @details Resets whenever a m_resize actually changes the allocation; helpful for spotting hot code that polls the allocator.
	 */
	size_t current_noop_resize_streak;
	/**
	 * @brief Longest observed streak of consecutive no-op resizes.
	 * @details Provides context for @ref current_noop_resize_streak, showing whether redundant m_resize storms occur.
	 */
	size_t peak_noop_resize_streak;
	/**
	 * @brief Count of times the library had to append a missing string terminator.
	 * @details Each concatenation that writes an explicit '\0' increments this counter, exposing unsafe string sources.
	 */
	size_t string_terminator_injections;
	/**
	 * @brief Count of operations that converted a descriptor mode from data to string.
	 * @details Incremented only when `destination->is_string` changes from `false` to `true` after a successful operation.
	 */
	size_t data_to_string_conversions;
	/**
	 * @brief Count of operations that converted a destination descriptor from a string to generic data
	 * @details Incremented only when `destination->is_string` changes from `true` to `false` after a successful operation
	 */
	size_t string_to_data_conversions;
	/**
	 * @brief Number of descriptors that currently own heap memory.
	 * @details Moves in tandem with successful first-time allocations and deletions, mirroring the live descriptor set.
	 */
	size_t current_active_descriptors;
	/**
	 * @brief Historical maximum of simultaneously active descriptors.
	 * @details Useful for sizing allocators and estimating peak parallelism.
	 */
	size_t peak_active_descriptors;
	/**
	 * @brief Count of arithmetic range violations prevented by guarded helpers.
	 * @details Every overflow or underflow caught by @ref mem_guarded_byte_size,
	 *          @ref mem_guarded_add, or @ref mem_guarded_subtract increments this counter
	 */
	size_t arithmetic_guard_failures;
} Telemetry;

extern Telemetry telemetry;

/**
 * @brief Increment @ref Telemetry::in_place_resizes.
 * @details Use when a m_resize request is satisfied without touching the OS allocator.
 */
void telemetry_in_place_resizes(void);

/**
 * @brief Increment @ref Telemetry::fresh_heap_allocations.
 * @details Use when previously unallocated memory grows from zero bytes.
 */
void telemetry_fresh_heap_allocations(void);

/**
 * @brief Increment @ref Telemetry::zero_initialized_payload_growths.
 * @details Use when a calloc-style growth occurs (new bytes are zeroed).
 */
void telemetry_zero_initialized_payload_growths(void);

/**
 * @brief Increment @ref Telemetry::heap_reallocations.
 * @details Use when `realloc` (or equivalent) is invoked with a non-zero delta.
 */
void telemetry_heap_reallocations(void);

/**
 * @brief Increment @ref Telemetry::noop_resizes.
 * @details Use when the requested logical size matches the current size exactly.
 */
void telemetry_noop_resizes(void);

/**
 * @brief Increment @ref Telemetry::heap_buffer_releases.
 * @details Use whenever memory is returned to the OS.
 */
void telemetry_heap_buffer_releases(void);

/**
 * @brief Increase @ref Telemetry::total_heap_reserved_bytes_released.
 * @param amount_of_bytes Number of bytes returned to the OS.
 */
void telemetry_total_heap_reserved_bytes_released(const size_t);

/**
 * @brief Increment @ref Telemetry::release_unused_shrinks.
 */
void telemetry_release_unused_shrinks(void);

/**
 * @brief Increase @ref Telemetry::total_release_unused_heap_reserved_bytes_released.
 * @param amount_of_bytes Number of bytes released in one shrink operation.
 */
void telemetry_total_release_unused_heap_reserved_bytes_released(const size_t);

/**
 * @brief Update heap-side metrics after acquiring bytes from the OS.
 * @param amount_of_bytes Amount of slab-rounded bytes newly obtained.
 */
void telemetry_heap_reserved_bytes_acquired(const size_t);

/**
 * @brief Update logical payload metrics after adding payload bytes.
 * @param amount_of_bytes Number of payload bytes that became addressable.
 */
void telemetry_payload_bytes_added(const size_t);

/**
 * @brief Reduce @ref Telemetry::current_heap_reserved_bytes after releasing bytes.
 * @param amount_of_bytes Amount of slab-rounded bytes returned.
 */
void telemetry_current_heap_reserved_bytes_released(const size_t);

/**
 * @brief Reduce @ref Telemetry::current_payload_bytes after removing payload bytes.
 * @param amount_of_bytes Number of payload bytes no longer addressable.
 */
void telemetry_current_payload_bytes_removed(const size_t);

/**
 * @brief Increment @ref Telemetry::heap_allocation_failures.
 */
void telemetry_heap_allocation_failures(void);

/**
 * @brief Increment @ref Telemetry::heap_reallocation_failures.
 */
void telemetry_heap_reallocation_failures(void);

/**
 * @brief Add bytes to block-overhead metrics.
 * @param amount_of_bytes Block-overhead bytes that were just added.
 */
void telemetry_block_overhead_bytes_added(const size_t);

/**
 * @brief Remove bytes from @ref Telemetry::current_block_overhead_bytes.
 * @param amount_of_bytes Block-overhead bytes that were just returned.
 */
void telemetry_current_block_overhead_bytes_removed(const size_t);

/**
 * @brief Advance no-op resize streak metrics.
 */
void telemetry_noop_resize_streak_advanced(void);

/**
 * @brief Reset @ref Telemetry::current_noop_resize_streak.
 */
void telemetry_current_noop_resize_streak_reset(void);

/**
 * @brief Increment @ref Telemetry::string_terminator_injections.
 */
void telemetry_string_terminator_injections(void);

/**
 * @brief Increment @ref Telemetry::data_to_string_conversions.
 */
void telemetry_data_to_string_conversions(void);

/**
 * @brief Increment @ref Telemetry::string_to_data_conversions
 */
void telemetry_string_to_data_conversions(void);

/**
 * @brief Update active-descriptor metrics after acquiring heap memory.
 */
void telemetry_active_descriptors_acquired(void);

/**
 * @brief Reduce @ref Telemetry::current_active_descriptors after releasing heap memory.
 */
void telemetry_active_descriptors_released(void);

/**
 * @brief Increment @ref Telemetry::arithmetic_guard_failures
 */
void telemetry_arithmetic_guard_failures(void);

/**
 * @brief Print the final telemetry summary at program shutdown
 *
 * Counters that are expected to drop back to zero by the end of the run
 * (current_heap_reserved_bytes, current_payload_bytes) are highlighted in green when they reached zero
 * and in red when they did not, so a leftover allocation or payload imbalance is immediately visible
 */
void telemetry_final_summary(void);

/**
 * @brief Reset all telemetry counters to zero.
 */
void init_telemetry(void);
