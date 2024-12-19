#include "mem.h"

void telemetry_realloc_optimized_counter(void)
{
	__atomic_fetch_add(&telemetry.realloc_optimized_counter, 1, __ATOMIC_SEQ_CST);
}

void telemetry_allocations_counter(void)
{
	__atomic_fetch_add(&telemetry.new_allocations_counter, 1, __ATOMIC_SEQ_CST);
}

void telemetry_aligned_reallocations_counter(void)
{
	__atomic_fetch_add(&telemetry.aligned_reallocations_counter, 1, __ATOMIC_SEQ_CST);
}

void telemetry_realloc_noop_counter(void)
{
	__atomic_fetch_add(&telemetry.realloc_noop_counter, 1, __ATOMIC_SEQ_CST);
}

void telemetry_free_counter(void)
{
	__atomic_fetch_add(&telemetry.free_counter, 1, __ATOMIC_SEQ_CST);
	#if SHOW
	printf("telemetry.free_counter: %zu\n",telemetry.free_counter);
	#endif
}

void telemetry_free_total_bytes(
	const size_t amount_of_bytes
){
	__atomic_fetch_add(&telemetry.free_total_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);
	#if SHOW
	printf("telemetry.free_total_bytes: %zu\n",telemetry.free_total_bytes);
	#endif
}

void telemetry_add(
	const size_t amount_of_bytes
)
{
	__atomic_fetch_add(&telemetry.current_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&telemetry.total_allocated_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);

	if(telemetry.current_bytes > telemetry.peak_bytes)
	{
		__atomic_store_n(&telemetry.peak_bytes, telemetry.current_bytes, __ATOMIC_SEQ_CST);
	}
	#if SHOW
	printf("+%ld\n",amount_of_bytes);
	#endif
}

void telemetry_effective_add(
	const size_t amount_of_bytes
)
{
	__atomic_fetch_add(&telemetry.total_effective_allocated_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&telemetry.current_effective_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);

	#if SHOW
	printf("+%ld\n",amount_of_bytes);
	#endif
}

void telemetry_reduce(
	const size_t amount_of_bytes
){
	__sync_sub_and_fetch(&telemetry.current_bytes,amount_of_bytes);
	#if SHOW
	printf("-%ld\n",amount_of_bytes);
	#endif
}

void telemetry_effective_reduce(
	const size_t amount_of_bytes
){
	__sync_sub_and_fetch(&telemetry.current_effective_bytes,amount_of_bytes);
	#if SHOW
	printf("-%ld\n",amount_of_bytes);
	#endif
}

void init_telemetry(void)
{
	memset(&telemetry,0x0,sizeof(Telemetry));
}

void telemetry_show(void)
{
	printf("Telemetry: Current memory bytes (should be 0): %s\n",bkbmbgbtbpbeb(telemetry.current_bytes));
	printf("Telemetry: Current effective memory bytes (should be 0): %s\n",bkbmbgbtbpbeb(telemetry.current_effective_bytes));
	printf("Telemetry: Peak memory allocation from the OS during program execution: %s\n",bkbmbgbtbpbeb(telemetry.peak_bytes));
	printf("Telemetry: Total memory free number: %s\n",form(telemetry.free_counter));
	printf("Telemetry: Total memory free bytes: %s\n",bkbmbgbtbpbeb(telemetry.free_total_bytes));
	printf("Telemetry: Total memory new allocations number: %s\n",form(telemetry.new_allocations_counter));
	printf("Telemetry: Total optimized trick memory reallocations number: %s\n",form(telemetry.realloc_optimized_counter));
	printf("Telemetry: Total aligned memory reallocations number: %s\n",form(telemetry.aligned_reallocations_counter));
	printf("Telemetry: Total aligned bytes allocated during execution: %s\n",bkbmbgbtbpbeb(telemetry.total_allocated_bytes));
	printf("Telemetry: Total effective memory allocated during execution: %s\n",bkbmbgbtbpbeb(telemetry.total_effective_allocated_bytes));
	printf("Telemetry: Count of no-op reallocations: %zu\n",telemetry.realloc_noop_counter);
}
