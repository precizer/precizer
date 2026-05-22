#include "mem.h"

Telemetry telemetry = {0};

#ifndef ROUGH_DEBUG
#define ROUGH_DEBUG 0
#endif

/**
 * @brief Update the peak heap usage counter if @p updated_current_bytes exceeds it.
 *
 * @param updated_current_bytes Newly observed heap usage value.
 */
static void telemetry_peak_heap_reserved_bytes_update(const size_t updated_current_bytes)
{
	size_t observed_peak = __atomic_load_n(&telemetry.peak_heap_reserved_bytes,__ATOMIC_SEQ_CST);

	while(updated_current_bytes > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_heap_reserved_bytes,
			&observed_peak,
			updated_current_bytes,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST))
		{
			break;
		}
	}
}

/**
 * @brief Track the maximum block overhead observed at runtime.
 *
 * @param updated_current_overhead Current block-overhead consumption in bytes.
 */
static void telemetry_peak_block_overhead_bytes_update(const size_t updated_current_overhead)
{
	size_t observed_peak = __atomic_load_n(&telemetry.peak_block_overhead_bytes,__ATOMIC_SEQ_CST);

	while(updated_current_overhead > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_block_overhead_bytes,
			&observed_peak,
			updated_current_overhead,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST))
		{
			break;
		}
	}
}

/**
 * @brief Record the highest number of simultaneously active descriptors.
 *
 * @param updated_current_active_descriptors Current count of active descriptors.
 */
static void telemetry_peak_active_descriptors_update(const size_t updated_current_active_descriptors)
{
	size_t observed_peak = __atomic_load_n(&telemetry.peak_active_descriptors,__ATOMIC_SEQ_CST);

	while(updated_current_active_descriptors > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_active_descriptors,
			&observed_peak,
			updated_current_active_descriptors,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST))
		{
			break;
		}
	}
}

void telemetry_in_place_resizes(void)
{
	__atomic_fetch_add(&telemetry.in_place_resizes,1,__ATOMIC_SEQ_CST);
}

void telemetry_fresh_heap_allocations(void)
{
	__atomic_fetch_add(&telemetry.fresh_heap_allocations,1,__ATOMIC_SEQ_CST);
}

void telemetry_zero_initialized_payload_growths(void)
{
	__atomic_fetch_add(&telemetry.zero_initialized_payload_growths,1,__ATOMIC_SEQ_CST);
}

void telemetry_heap_reallocations(void)
{
	__atomic_fetch_add(&telemetry.heap_reallocations,1,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.heap_reallocations: %zu\n",telemetry.heap_reallocations);
	#endif
}

void telemetry_noop_resizes(void)
{
	__atomic_fetch_add(&telemetry.noop_resizes,1,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.noop_resizes: %zu\n",telemetry.noop_resizes);
	#endif
}

void telemetry_heap_buffer_releases(void)
{
	__atomic_fetch_add(&telemetry.heap_buffer_releases,1,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.heap_buffer_releases: %zu\n",telemetry.heap_buffer_releases);
	#endif
}

void telemetry_total_heap_reserved_bytes_released(const size_t amount_of_bytes)
{
	__atomic_fetch_add(&telemetry.total_heap_reserved_bytes_released,amount_of_bytes,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.total_heap_reserved_bytes_released: %zu\n",telemetry.total_heap_reserved_bytes_released);
	#endif
}

void telemetry_release_unused_shrinks(void)
{
	__atomic_fetch_add(&telemetry.release_unused_shrinks,1,__ATOMIC_SEQ_CST);
}

void telemetry_total_release_unused_heap_reserved_bytes_released(const size_t amount_of_bytes)
{
	if(amount_of_bytes == 0)
	{
		return;
	}

	__atomic_fetch_add(&telemetry.total_release_unused_heap_reserved_bytes_released,amount_of_bytes,__ATOMIC_SEQ_CST);
}

void telemetry_heap_reserved_bytes_acquired(const size_t amount_of_bytes)
{
	const size_t updated_current_bytes = __atomic_add_fetch(
		&telemetry.current_heap_reserved_bytes,
		amount_of_bytes,
		__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&telemetry.total_heap_reserved_bytes_acquired,amount_of_bytes,__ATOMIC_SEQ_CST);
	telemetry_peak_heap_reserved_bytes_update(updated_current_bytes);

	#if ROUGH_DEBUG
	printf("+%zu\n",amount_of_bytes);
	#endif
}

void telemetry_payload_bytes_added(const size_t amount_of_bytes)
{
	__atomic_fetch_add(&telemetry.total_payload_bytes_added,amount_of_bytes,__ATOMIC_SEQ_CST);
	__atomic_add_fetch(&telemetry.current_payload_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);

	#if ROUGH_DEBUG
	printf("+%zu\n",amount_of_bytes);
	#endif
}

void telemetry_current_heap_reserved_bytes_released(const size_t amount_of_bytes)
{
	__atomic_sub_fetch(&telemetry.current_heap_reserved_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("-%zu\n",amount_of_bytes);
	#endif
}

void telemetry_current_payload_bytes_removed(const size_t amount_of_bytes)
{
	__atomic_sub_fetch(&telemetry.current_payload_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("-%zu\n",amount_of_bytes);
	#endif
}

void telemetry_heap_allocation_failures(void)
{
	__atomic_fetch_add(&telemetry.heap_allocation_failures,1,__ATOMIC_SEQ_CST);
}

void telemetry_expected_heap_allocation_failures(void)
{
	__atomic_fetch_add(&telemetry.expected_heap_allocation_failures,1,__ATOMIC_SEQ_CST);
}

void telemetry_heap_reallocation_failures(void)
{
	__atomic_fetch_add(&telemetry.heap_reallocation_failures,1,__ATOMIC_SEQ_CST);
}

void telemetry_expected_heap_reallocation_failures(void)
{
	__atomic_fetch_add(&telemetry.expected_heap_reallocation_failures,1,__ATOMIC_SEQ_CST);
}

void telemetry_block_overhead_bytes_added(const size_t amount_of_bytes)
{
	if(amount_of_bytes == 0)
	{
		return;
	}

	const size_t updated_current_overhead = __atomic_add_fetch(
		&telemetry.current_block_overhead_bytes,
		amount_of_bytes,
		__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&telemetry.total_block_overhead_bytes_added,amount_of_bytes,__ATOMIC_SEQ_CST);
	telemetry_peak_block_overhead_bytes_update(updated_current_overhead);
}

void telemetry_current_block_overhead_bytes_removed(const size_t amount_of_bytes)
{
	if(amount_of_bytes == 0)
	{
		return;
	}

	__atomic_sub_fetch(&telemetry.current_block_overhead_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
}

void telemetry_consecutive_noop_resizes_advanced(void)
{
	const size_t updated_consecutive_noops = __atomic_add_fetch(
		&telemetry.current_consecutive_noop_resizes,
		1,
		__ATOMIC_SEQ_CST);

	size_t observed_peak = __atomic_load_n(&telemetry.peak_consecutive_noop_resizes,__ATOMIC_SEQ_CST);

	while(updated_consecutive_noops > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_consecutive_noop_resizes,
			&observed_peak,
			updated_consecutive_noops,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST))
		{
			break;
		}
	}
}

void telemetry_current_consecutive_noop_resizes_reset(void)
{
	__atomic_store_n(&telemetry.current_consecutive_noop_resizes,0,__ATOMIC_SEQ_CST);
}

void telemetry_data_to_string_conversions(void)
{
	__atomic_fetch_add(&telemetry.data_to_string_conversions,1,__ATOMIC_SEQ_CST);
}

void telemetry_string_to_data_conversions(void)
{
	__atomic_fetch_add(&telemetry.string_to_data_conversions,1,__ATOMIC_SEQ_CST);
}

void telemetry_finalize_string_terminator_already_present(void)
{
	__atomic_fetch_add(&telemetry.finalize_string_terminator_already_present,1,__ATOMIC_SEQ_CST);
}

void telemetry_finalize_string_terminator_written_when_missing(void)
{
	__atomic_fetch_add(&telemetry.finalize_string_terminator_written_when_missing,1,__ATOMIC_SEQ_CST);
}

void telemetry_string_terminator_writes(void)
{
	__atomic_fetch_add(&telemetry.string_terminator_writes,1,__ATOMIC_SEQ_CST);
}

void telemetry_active_descriptors_acquired(void)
{
	const size_t updated_active = __atomic_add_fetch(
		&telemetry.current_active_descriptors,
		1,
		__ATOMIC_SEQ_CST);

	telemetry_peak_active_descriptors_update(updated_active);
}

void telemetry_active_descriptors_released(void)
{
	__atomic_sub_fetch(&telemetry.current_active_descriptors,1,__ATOMIC_SEQ_CST);
}

void telemetry_arithmetic_guard_failures(void)
{
	__atomic_fetch_add(&telemetry.arithmetic_guard_failures,1,__ATOMIC_SEQ_CST);
}

void telemetry_expected_arithmetic_guard_failures(void)
{
	__atomic_fetch_add(&telemetry.expected_arithmetic_guard_failures,1,__ATOMIC_SEQ_CST);
}
