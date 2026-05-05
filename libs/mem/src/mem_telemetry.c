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
static void telemetry_update_peak(const size_t updated_current_bytes)
{
	size_t observed_peak = __atomic_load_n(&telemetry.peak_heap_bytes,__ATOMIC_SEQ_CST);

	while(updated_current_bytes > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_heap_bytes,
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
 * @brief Track the maximum alignment overhead observed at runtime.
 *
 * @param updated_current_overhead Current alignment padding consumption in bytes.
 */
static void telemetry_update_alignment_peak(const size_t updated_current_overhead)
{
	size_t observed_peak = __atomic_load_n(&telemetry.peak_alignment_overhead_bytes,__ATOMIC_SEQ_CST);

	while(updated_current_overhead > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_alignment_overhead_bytes,
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
 * @param updated_active_descriptors Current count of active descriptors.
 */
static void telemetry_update_active_peak(const size_t updated_active_descriptors)
{
	size_t observed_peak = __atomic_load_n(&telemetry.peak_active_descriptors,__ATOMIC_SEQ_CST);

	while(updated_active_descriptors > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_active_descriptors,
			&observed_peak,
			updated_active_descriptors,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST))
		{
			break;
		}
	}
}

void telemetry_realloc_optimized_counter(void)
{
	__atomic_fetch_add(&telemetry.optimized_resizes_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_new_allocations_counter(void)
{
	__atomic_fetch_add(&telemetry.fresh_allocations_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_new_callocations_counter(void)
{
	__atomic_fetch_add(&telemetry.zero_initialized_allocations_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_aligned_reallocations_counter(void)
{
	__atomic_fetch_add(&telemetry.heap_reallocations_counter,1,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.heap_reallocations_counter: %zu\n",telemetry.heap_reallocations_counter);
	#endif
}

void telemetry_realloc_noop_counter(void)
{
	__atomic_fetch_add(&telemetry.exact_size_resizes_counter,1,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.exact_size_resizes_counter: %zu\n",telemetry.exact_size_resizes_counter);
	#endif
}

void telemetry_free_counter(void)
{
	__atomic_fetch_add(&telemetry.release_operations_counter,1,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.release_operations_counter: %zu\n",telemetry.release_operations_counter);
	#endif
}

void telemetry_free_total_bytes(const size_t amount_of_bytes)
{
	__atomic_fetch_add(&telemetry.total_heap_bytes_released,amount_of_bytes,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("telemetry.total_heap_bytes_released: %zu\n",telemetry.total_heap_bytes_released);
	#endif
}

void telemetry_release_unused_operation(void)
{
	__atomic_fetch_add(&telemetry.release_unused_operations_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_release_unused_bytes(const size_t amount_of_bytes)
{
	if(amount_of_bytes == 0)
	{
		return;
	}

	__atomic_fetch_add(&telemetry.release_unused_bytes_total,amount_of_bytes,__ATOMIC_SEQ_CST);
}

void telemetry_add(const size_t amount_of_bytes)
{
	const size_t updated_current_bytes = __atomic_add_fetch(
		&telemetry.current_heap_bytes,
		amount_of_bytes,
		__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&telemetry.total_heap_bytes_acquired,amount_of_bytes,__ATOMIC_SEQ_CST);
	telemetry_update_peak(updated_current_bytes);

	#if ROUGH_DEBUG
	printf("+%zu\n",amount_of_bytes);
	#endif
}

void telemetry_effective_add(const size_t amount_of_bytes)
{
	__atomic_fetch_add(&telemetry.total_payload_bytes_acquired,amount_of_bytes,__ATOMIC_SEQ_CST);
	__atomic_add_fetch(&telemetry.current_payload_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);

	#if ROUGH_DEBUG
	printf("+%zu\n",amount_of_bytes);
	#endif
}

void telemetry_reduce(const size_t amount_of_bytes)
{
	__atomic_sub_fetch(&telemetry.current_heap_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("-%zu\n",amount_of_bytes);
	#endif
}

void telemetry_effective_reduce(const size_t amount_of_bytes)
{
	__atomic_sub_fetch(&telemetry.current_payload_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
	#if ROUGH_DEBUG
	printf("-%zu\n",amount_of_bytes);
	#endif
}

void telemetry_allocation_failure(void)
{
	__atomic_fetch_add(&telemetry.heap_allocation_failures_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_reallocation_failure(void)
{
	__atomic_fetch_add(&telemetry.heap_reallocation_failures_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_alignment_overhead_add(const size_t amount_of_bytes)
{
	if(amount_of_bytes == 0)
	{
		return;
	}

	const size_t updated_current_overhead = __atomic_add_fetch(
		&telemetry.current_alignment_overhead_bytes,
		amount_of_bytes,
		__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&telemetry.total_alignment_overhead_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
	telemetry_update_alignment_peak(updated_current_overhead);
}

void telemetry_alignment_overhead_reduce(const size_t amount_of_bytes)
{
	if(amount_of_bytes == 0)
	{
		return;
	}

	__atomic_sub_fetch(&telemetry.current_alignment_overhead_bytes,amount_of_bytes,__ATOMIC_SEQ_CST);
}

void telemetry_noop_resize_event(void)
{
	const size_t updated_streak = __atomic_add_fetch(
		&telemetry.noop_resize_streak_current,
		1,
		__ATOMIC_SEQ_CST);

	size_t observed_peak = __atomic_load_n(&telemetry.noop_resize_streak_peak,__ATOMIC_SEQ_CST);

	while(updated_streak > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.noop_resize_streak_peak,
			&observed_peak,
			updated_streak,
			false,
			__ATOMIC_SEQ_CST,
			__ATOMIC_SEQ_CST))
		{
			break;
		}
	}
}

void telemetry_reset_noop_streak(void)
{
	__atomic_store_n(&telemetry.noop_resize_streak_current,0,__ATOMIC_SEQ_CST);
}

void telemetry_string_padding_event(void)
{
	__atomic_fetch_add(&telemetry.concat_zero_padding_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_descriptor_mode_converted_to_string(void)
{
	__atomic_fetch_add(&telemetry.descriptor_mode_converted_to_string_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_destination_promoted_to_data(void)
{
	__atomic_fetch_add(&telemetry.destination_promoted_to_data_counter,1,__ATOMIC_SEQ_CST);
}

void telemetry_active_descriptor_acquire(void)
{
	const size_t updated_active = __atomic_add_fetch(
		&telemetry.active_descriptors,
		1,
		__ATOMIC_SEQ_CST);

	telemetry_update_active_peak(updated_active);
}

void telemetry_active_descriptor_release(void)
{
	__atomic_sub_fetch(&telemetry.active_descriptors,1,__ATOMIC_SEQ_CST);
}

void telemetry_arithmetic_guard_failure(void)
{
	__atomic_fetch_add(&telemetry.arithmetic_guard_failures_counter,1,__ATOMIC_SEQ_CST);
}

void init_telemetry(void)
{
	memset(&telemetry,0x0,sizeof(Telemetry));
}

void telemetry_show(void)
{
	char buf[FORM_OUTPUT_BUFFER_SIZE];

	printf("Telemetry: Outstanding heap bytes (expected 0): %s\n",bkbmbgbtbpbeb(telemetry.current_heap_bytes,FULL_VIEW));
	printf("Telemetry: Outstanding payload bytes (expected 0): %s\n",bkbmbgbtbpbeb(telemetry.current_payload_bytes,FULL_VIEW));
	printf("Telemetry: Peak heap footprint: %s\n",bkbmbgbtbpbeb(telemetry.peak_heap_bytes,FULL_VIEW));
	printf("Telemetry: Free operations count: %s\n",form(telemetry.release_operations_counter,buf,sizeof(buf)));
	printf("Telemetry: Bytes released to the OS: %s\n",bkbmbgbtbpbeb(telemetry.total_heap_bytes_released,FULL_VIEW));
	printf("Telemetry: Shrink calls that forced immediate buffer release: %s\n",form(telemetry.release_unused_operations_counter,buf,sizeof(buf)));
	printf("Telemetry: Bytes returned by those forced releases: %s\n",bkbmbgbtbpbeb(telemetry.release_unused_bytes_total,FULL_VIEW));
	printf("Telemetry: Fresh allocation count: %s\n",form(telemetry.fresh_allocations_counter,buf,sizeof(buf)));
	printf("Telemetry: Zero-initialized allocation count: %s\n",form(telemetry.zero_initialized_allocations_counter,buf,sizeof(buf)));
	printf("Telemetry: Optimized resize count: %s\n",form(telemetry.optimized_resizes_counter,buf,sizeof(buf)));
	printf("Telemetry: Realignment resize count: %s\n",form(telemetry.heap_reallocations_counter,buf,sizeof(buf)));
	printf("Telemetry: Total aligned bytes requested: %s\n",bkbmbgbtbpbeb(telemetry.total_heap_bytes_acquired,FULL_VIEW));
	printf("Telemetry: Total payload bytes requested: %s\n",bkbmbgbtbpbeb(telemetry.total_payload_bytes_acquired,FULL_VIEW));
	printf("Telemetry: Exact-size resize count: %s\n",form(telemetry.exact_size_resizes_counter,buf,sizeof(buf)));
	printf("Telemetry: Allocation failures intercepted: %s\n",form(telemetry.heap_allocation_failures_counter,buf,sizeof(buf)));
	printf("Telemetry: Reallocation failures intercepted: %s\n",form(telemetry.heap_reallocation_failures_counter,buf,sizeof(buf)));
	printf("Telemetry: Current alignment overhead: %s\n",bkbmbgbtbpbeb(telemetry.current_alignment_overhead_bytes,FULL_VIEW));
	printf("Telemetry: Peak alignment overhead: %s\n",bkbmbgbtbpbeb(telemetry.peak_alignment_overhead_bytes,FULL_VIEW));
	printf("Telemetry: Total alignment overhead accrued: %s\n",bkbmbgbtbpbeb(telemetry.total_alignment_overhead_bytes,FULL_VIEW));
	printf("Telemetry: Current no-op resize streak: %s\n",form(telemetry.noop_resize_streak_current,buf,sizeof(buf)));
	printf("Telemetry: Longest no-op resize streak: %s\n",form(telemetry.noop_resize_streak_peak,buf,sizeof(buf)));
	printf("Telemetry: String padding injections: %s\n",form(telemetry.concat_zero_padding_counter,buf,sizeof(buf)));
	printf("Telemetry: Descriptor mode conversions to string: %s\n",form(telemetry.descriptor_mode_converted_to_string_counter,buf,sizeof(buf)));
	printf("Telemetry: Destination promotions to data: %s\n",form(telemetry.destination_promoted_to_data_counter,buf,sizeof(buf)));
	printf("Telemetry: Active descriptors: %s\n",form(telemetry.active_descriptors,buf,sizeof(buf)));
	printf("Telemetry: Peak active descriptors: %s\n",form(telemetry.peak_active_descriptors,buf,sizeof(buf)));
	printf("Telemetry: Arithmetic guards triggered: %s\n",form(telemetry.arithmetic_guard_failures_counter,buf,sizeof(buf)));
}
