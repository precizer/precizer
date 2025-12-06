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
	 * @brief Total aligned bytes currently owned by every descriptor.
	 * @details Grows whenever the helper acquires memory from the OS and shrinks only
	 *          when a descriptor releases its buffer via @ref memory_delete or `resize(...,0)`.
	 */
	size_t current_heap_bytes;
	/**
	 * @brief Logical payload bytes that callers can actually use.
	 * @details Equals `sum(length * element_size)` across descriptors and ignores
	 *          alignment padding so that business logic can track real data pressure.
	 */
	size_t current_payload_bytes;
	/**
	 * @brief Count of descriptors that grew from zero bytes to a non-zero allocation.
	 * @details Highlights how often brand-new buffers are provisioned (independent of realloc).
	 */
	size_t fresh_allocations_counter;
	/**
	 * @brief Count of calloc-style growth events.
	 * @details Incremented when the helper both allocates fresh memory and zeroes it (e.g., first resize after create + calloc semantics).
	 */
	size_t zero_initialized_allocations_counter;
	/**
	 * @brief Cumulative aligned bytes ever requested from the OS allocator.
	 * @details Useful to estimate the real heap footprint over the lifetime of the process.
	 */
	size_t total_heap_bytes_acquired;
	/**
	 * @brief Cumulative logical payload bytes that user code asked for.
	 * @details Helps compare requested payload versus actual heap usage (see alignment metrics).
	 */
	size_t total_payload_bytes_acquired;
	/**
	 * @brief Count of reallocations that delegated to the OS allocator.
	 * @details Includes both growth and shrink requests that required `realloc` rather than metadata-only adjustments.
	 */
	size_t heap_reallocations_counter;
	/**
	 * @brief Count of resizes served without touching the OS.
	 * @details Tracks optimized cases where existing capacity was sufficient and only bookkeeping changed.
	 */
	size_t optimized_resizes_counter;
	/**
	 * @brief Number of times buffers were freed back to the operating system.
	 * @details Each successful `memory_delete` contributes exactly once, enabling leak detection.
	 */
	size_t release_operations_counter;
	/**
	 * @brief Total bytes returned to the OS.
	 * @details Complements @ref total_heap_bytes_acquired to show how much memory was recycled.
	 */
	size_t total_heap_bytes_released;
	/**
	 * @brief Count of shrink operations performed due to @ref RELEASE_UNUSED.
	 * @details Tracks how often callers requested aggressive trimming of buffers.
	 */
	size_t release_unused_operations_counter;
	/**
	 * @brief Total bytes returned specifically via @ref RELEASE_UNUSED.
	 * @details Helps quantify the effectiveness of aggressive shrink requests.
	 */
	size_t release_unused_bytes_total;
	/**
	 * @brief Historical maximum of @ref current_heap_bytes.
	 * @details Indicates the highest simultaneous heap footprint achieved by the helper.
	 */
	size_t peak_heap_bytes;
	/**
	 * @brief Count of resize requests that exactly matched the current logical size.
	 * @details Correlates with redundant `resize` calls and feeds the no-op streak metrics.
	 */
	size_t exact_size_resizes_counter;
	/**
	 * @brief Number of failed `malloc`/`calloc` style calls.
	 * @details Allows operators to differentiate between logic issues and genuine resource exhaustion.
	 */
	size_t heap_allocation_failures_counter;
	/**
	 * @brief Number of failed `realloc` attempts.
	 * @details Highlights pressure scenarios where existing buffers could not grow because the OS refused the request.
	 */
	size_t heap_reallocation_failures_counter;
	/**
	 * @brief Alignment padding currently wasted across all descriptors.
	 * @details Equals `sum(aligned_bytes - payload_bytes)` and exposes how much memory sits idle due to page alignment.
	 */
	size_t current_alignment_overhead_bytes;
	/**
	 * @brief Cumulative alignment padding ever observed.
	 * @details Monotonic counter that grows whenever additional padding becomes necessary, useful for budgeting fragmentation.
	 */
	size_t total_alignment_overhead_bytes;
	/**
	 * @brief Historical maximum of @ref current_alignment_overhead_bytes.
	 * @details Shows the worst-case alignment slack simultaneously held in memory.
	 */
	size_t peak_alignment_overhead_bytes;
	/**
	 * @brief Current length of the latest consecutive no-op resize streak.
	 * @details Resets whenever a resize actually changes the allocation; helpful for spotting hot code that polls the allocator.
	 */
	size_t noop_resize_streak_current;
	/**
	 * @brief Longest observed streak of consecutive no-op resizes.
	 * @details Provides context for @ref noop_resize_streak_current, showing whether redundant resize storms occur.
	 */
	size_t noop_resize_streak_peak;
	/**
	 * @brief Count of times the library had to append a missing string terminator.
	 * @details Each concatenation that writes an explicit '\0' increments this counter, exposing unsafe string sources.
	 */
	size_t concat_zero_padding_counter;
	/**
	 * @brief Number of descriptors that currently own heap memory.
	 * @details Moves in tandem with successful first-time allocations and deletions, mirroring the live descriptor set.
	 */
	size_t active_descriptors;
	/**
	 * @brief Historical maximum of simultaneously active descriptors.
	 * @details Useful for sizing allocators and estimating peak parallelism.
	 */
	size_t peak_active_descriptors;
	/**
	 * @brief Count of arithmetic overflows prevented by @ref memory_guarded_size.
	 * @details Every overflow that is caught before it reaches the allocator increments this counter, proving the safety net works.
	 */
	size_t overflow_guard_failures_counter;
} Telemetry;

extern Telemetry telemetry;

/**
 * @brief Increment the optimized reallocation counter.
 * @details Use when a resize request is satisfied without touching the OS allocator.
 */
void telemetry_realloc_optimized_counter(void);

/**
 * @brief Increment the counter for fresh allocations.
 * @details Use when previously unallocated memory grows from zero bytes.
 */
void telemetry_new_allocations_counter(void);

/**
 * @brief Increment the counter for zero-initialized allocations.
 * @details Use when a calloc-style growth occurs (new bytes are zeroed).
 */
void telemetry_new_callocations_counter(void);

/**
 * @brief Increment the counter for reallocations that hit the OS allocator.
 * @details Use when `realloc` (or equivalent) is invoked with a non-zero delta.
 */
void telemetry_aligned_reallocations_counter(void);

/**
 * @brief Increment the counter for no-op resize requests.
 * @details Use when the requested logical size matches the current size exactly.
 */
void telemetry_realloc_noop_counter(void);

/**
 * @brief Increment the counter for free operations.
 * @details Use whenever memory is returned to the OS.
 */
void telemetry_free_counter(void);

/**
 * @brief Accumulate how many bytes were freed.
 * @param amount_of_bytes Number of bytes returned to the OS.
 */
void telemetry_free_total_bytes(const size_t);

/**
 * @brief Increment the counter for RELEASE_UNUSED-driven shrink requests.
 */
void telemetry_release_unused_operation(void);

/**
 * @brief Accumulate bytes returned via RELEASE_UNUSED-driven shrink requests.
 * @param amount_of_bytes Number of bytes released in one shrink operation.
 */
void telemetry_release_unused_bytes(const size_t amount_of_bytes);

/**
 * @brief Update heap-side metrics after allocating bytes from the OS.
 * @param amount_of_bytes Amount of aligned bytes newly obtained.
 */
void telemetry_add(const size_t);

/**
 * @brief Update logical payload metrics after growing the element count.
 * @param amount_of_bytes Number of payload bytes that became addressable.
 */
void telemetry_effective_add(const size_t amount_of_bytes);

/**
 * @brief Update heap metrics after releasing bytes to the OS.
 * @param amount_of_bytes Amount of aligned bytes returned.
 */
void telemetry_reduce(const size_t amount_of_bytes);

/**
 * @brief Update logical payload metrics after shrinking the element count.
 * @param amount_of_bytes Number of payload bytes no longer addressable.
 */
void telemetry_effective_reduce(const size_t amount_of_bytes);

/**
 * @brief Record a failed allocation attempt.
 */
void telemetry_allocation_failure(void);

/**
 * @brief Record a failed reallocation attempt.
 */
void telemetry_reallocation_failure(void);

/**
 * @brief Increase the tracked alignment padding usage.
 * @param amount_of_bytes Alignment overhead bytes that were just added.
 */
void telemetry_alignment_overhead_add(const size_t amount_of_bytes);

/**
 * @brief Decrease the tracked alignment padding usage.
 * @param amount_of_bytes Alignment overhead bytes that were just returned.
 */
void telemetry_alignment_overhead_reduce(const size_t amount_of_bytes);

/**
 * @brief Track that a resize resulted in no actual change.
 */
void telemetry_noop_resize_event(void);

/**
 * @brief Reset the consecutive no-op resize streak counter.
 */
void telemetry_reset_noop_streak(void);

/**
 * @brief Record that the concatenation helper had to inject a string terminator.
 */
void telemetry_string_padding_event(void);

/**
 * @brief Track that a descriptor has acquired heap memory.
 */
void telemetry_active_descriptor_acquire(void);

/**
 * @brief Track that a descriptor has released its heap memory.
 */
void telemetry_active_descriptor_release(void);

/**
 * @brief Record an overflow that was caught by memory_guarded_size.
 */
void telemetry_overflow_guard_failure(void);

/**
 * @brief Print a human-readable snapshot of telemetry counters.
 */
void telemetry_show(void);

/**
 * @brief Reset all telemetry counters to zero.
 */
void init_telemetry(void);
