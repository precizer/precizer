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

void telemetry_heap_reallocation_failures(void)
{
	__atomic_fetch_add(&telemetry.heap_reallocation_failures,1,__ATOMIC_SEQ_CST);
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

void telemetry_noop_resize_streak_advanced(void)
{
	const size_t updated_streak = __atomic_add_fetch(
		&telemetry.current_noop_resize_streak,
		1,
		__ATOMIC_SEQ_CST);

	size_t observed_peak = __atomic_load_n(&telemetry.peak_noop_resize_streak,__ATOMIC_SEQ_CST);

	while(updated_streak > observed_peak)
	{
		if(__atomic_compare_exchange_n(
			&telemetry.peak_noop_resize_streak,
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

void telemetry_current_noop_resize_streak_reset(void)
{
	__atomic_store_n(&telemetry.current_noop_resize_streak,0,__ATOMIC_SEQ_CST);
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

void telemetry_final_summary(void)
{
	char buf[FORM_OUTPUT_BUFFER_SIZE];

	printf(BOLD "Memory balance" RESET WHITE "\n");
	printf("Current heap reserved bytes (expected 0B), now is: %s%s" RESET WHITE "\n",(telemetry.current_heap_reserved_bytes == 0) ? BOLDGREEN : BOLDRED,bkbmbgbtbpbeb(telemetry.current_heap_reserved_bytes,FULL_VIEW));
	printf("Current payload bytes (expected 0B), now is: %s%s" RESET WHITE "\n",(telemetry.current_payload_bytes == 0) ? BOLDGREEN : BOLDRED,bkbmbgbtbpbeb(telemetry.current_payload_bytes,FULL_VIEW));
	printf("Current active descriptors (expected 0), now is: %s%s" RESET WHITE "\n",(telemetry.current_active_descriptors == 0) ? BOLDGREEN : BOLDRED,form(telemetry.current_active_descriptors,buf,sizeof(buf)));

	printf("\n" BOLD "Heap reserve" RESET WHITE "\n");
	printf("Peak heap reserved bytes: %s\n",bkbmbgbtbpbeb(telemetry.peak_heap_reserved_bytes,FULL_VIEW));
	printf("Total heap reserved bytes acquired: %s\n",bkbmbgbtbpbeb(telemetry.total_heap_reserved_bytes_acquired,FULL_VIEW));
	printf("Total heap reserved bytes released (expected %zuB), now is: %s%s" RESET WHITE "\n",telemetry.total_heap_reserved_bytes_acquired,(telemetry.total_heap_reserved_bytes_released == telemetry.total_heap_reserved_bytes_acquired) ? BOLDGREEN : BOLDRED,bkbmbgbtbpbeb(telemetry.total_heap_reserved_bytes_released,FULL_VIEW));
	printf("Fresh heap allocations: %s\n",form(telemetry.fresh_heap_allocations,buf,sizeof(buf)));
	printf("Heap reallocations: %s\n",form(telemetry.heap_reallocations,buf,sizeof(buf)));
	printf("Heap buffer releases (expected %zu), now is: %s%s" RESET WHITE "\n",telemetry.fresh_heap_allocations,(telemetry.heap_buffer_releases == telemetry.fresh_heap_allocations) ? BOLDGREEN : BOLDRED,form(telemetry.heap_buffer_releases,buf,sizeof(buf)));
	printf("Heap allocation failures: %s\n",form(telemetry.heap_allocation_failures,buf,sizeof(buf)));
	printf("Heap reallocation failures: %s\n",form(telemetry.heap_reallocation_failures,buf,sizeof(buf)));

	printf("\n" BOLD "Payload" RESET WHITE "\n");
	printf("Total payload bytes added: %s\n",bkbmbgbtbpbeb(telemetry.total_payload_bytes_added,FULL_VIEW));
	printf("Zero-initialized payload growths: %s\n",form(telemetry.zero_initialized_payload_growths,buf,sizeof(buf)));

	printf("\n" BOLD "Release-unused" RESET WHITE "\n");
	printf("Release-unused shrinks: %s\n",form(telemetry.release_unused_shrinks,buf,sizeof(buf)));
	printf("Total release-unused heap reserved bytes released: %s\n",bkbmbgbtbpbeb(telemetry.total_release_unused_heap_reserved_bytes_released,FULL_VIEW));

	printf("\n" BOLD "Block overhead" RESET WHITE "\n");
	printf("Current block overhead bytes (expected 0B), now is: %s%s" RESET WHITE "\n",(telemetry.current_block_overhead_bytes == 0) ? BOLDGREEN : BOLDRED,bkbmbgbtbpbeb(telemetry.current_block_overhead_bytes,FULL_VIEW));
	printf("Peak block overhead bytes: %s\n",bkbmbgbtbpbeb(telemetry.peak_block_overhead_bytes,FULL_VIEW));
	printf("Total block overhead bytes added: %s\n",bkbmbgbtbpbeb(telemetry.total_block_overhead_bytes_added,FULL_VIEW));

	printf("\n" BOLD "Resize behavior" RESET WHITE "\n");
	printf("In-place resizes: %s\n",form(telemetry.in_place_resizes,buf,sizeof(buf)));
	printf("No-op resizes: %s\n",form(telemetry.noop_resizes,buf,sizeof(buf)));
	printf("Current no-op resize streak (expected 0), now is: %s%s" RESET WHITE "\n",(telemetry.current_noop_resize_streak == 0) ? BOLDGREEN : BOLDRED,form(telemetry.current_noop_resize_streak,buf,sizeof(buf)));
	printf("Peak no-op resize streak: %s\n",form(telemetry.peak_noop_resize_streak,buf,sizeof(buf)));

	printf("\n" BOLD "String and mode conversions" RESET WHITE "\n");
	printf("Data-to-string conversions: %s\n",form(telemetry.data_to_string_conversions,buf,sizeof(buf)));
	printf("String-to-data conversions: %s\n",form(telemetry.string_to_data_conversions,buf,sizeof(buf)));
	printf("Finalize string terminator already present: %s\n",form(telemetry.finalize_string_terminator_already_present,buf,sizeof(buf)));
	printf("Finalize string terminator written when missing: %s\n",form(telemetry.finalize_string_terminator_written_when_missing,buf,sizeof(buf)));
	printf("String terminator writes: %s\n",form(telemetry.string_terminator_writes,buf,sizeof(buf)));

	printf("\n" BOLD "Descriptor activity" RESET WHITE "\n");
	printf("Peak active descriptors: %s\n",form(telemetry.peak_active_descriptors,buf,sizeof(buf)));

	printf("\n" BOLD "Safety" RESET WHITE "\n");
	printf("Arithmetic guard failures: %s\n",form(telemetry.arithmetic_guard_failures,buf,sizeof(buf)));
}
