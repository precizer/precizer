#include "structs.h"
#include "mem.h"

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_trick_reallocations_counter(void)
{
	__atomic_fetch_add(&telemetry.heap_trick_reallocations_counter, 1, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_allocations_counter(void)
{
	__atomic_fetch_add(&telemetry.new_allocations_counter, 1, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_true_reallocations_counter(void)
{
	__atomic_fetch_add(&telemetry.heap_true_reallocations_counter, 1, __ATOMIC_SEQ_CST);
}


/**
 *
 * @brief
 * @details
 *
 */
void telemetry_how_many_times_realloc_do_nothing(void)
{
	__atomic_fetch_add(&telemetry.how_many_times_realloc_do_nothing, 1, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_free_counter(void)
{
	__atomic_fetch_add(&telemetry.free_counter, 1, __ATOMIC_SEQ_CST);
}

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_free_bytes(
	const size_t amount_of_bytes
){
	__atomic_fetch_add(&telemetry.free_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);
}


/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_add(
	const size_t amount_of_bytes
)
{
	__atomic_fetch_add(&telemetry.current_heap_memory_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&telemetry.total_heap_memory_allocated_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);

	if(telemetry.current_heap_memory_bytes > telemetry.max_heap_memory_bytes_are_utilized_at_the_same_time_instant)
	{
		__atomic_store_n(&telemetry.max_heap_memory_bytes_are_utilized_at_the_same_time_instant, telemetry.current_heap_memory_bytes, __ATOMIC_SEQ_CST);
	}
	#if 0
	printf("+%ld\n",amount_of_bytes);
	#endif
}

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_effective_add(
	const size_t amount_of_bytes
)
{
	__atomic_fetch_add(&telemetry.total_effective_memory_allocated_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&telemetry.current_effective_memory_bytes, amount_of_bytes, __ATOMIC_SEQ_CST);

	#if 0
	printf("+%ld\n",amount_of_bytes);
	#endif
}


/**
 *
 * @brief
 * @details
 *
 */
void telemetry_heap_reduce(
	const size_t amount_of_bytes
){
	__sync_sub_and_fetch(&telemetry.current_heap_memory_bytes,amount_of_bytes);
	#if 0
	printf("-%ld\n",amount_of_bytes);
	#endif
}

/**
 *
 * @brief
 * @details
 *
 */
void telemetry_effective_reduce(
	const size_t amount_of_bytes
){
	__sync_sub_and_fetch(&telemetry.current_effective_memory_bytes,amount_of_bytes);
	#if 0
	printf("-%ld\n",amount_of_bytes);
	#endif
}

/**
 *
 * @brief Инициализация телеметрии
 * @details Инициализация нулевыми значениями глобальной структуры
 * и вычислимых значений
 *
 */
void init_telemetry(void)
{
	// Телеметрия. Глобальная структура.
	// Инициализация структуры телеметрии нулевыми значениями
	memset(&telemetry,0x0,sizeof(Telemetry));
}

/**
 *
 * @brief Функция вывода финального состояния телеметрии
 *
 */
void telemetry_show(void)
{
	printf("Telemetry: Current heap memory (should be 0): %s\n",bkbmbgbtbpbeb(telemetry.current_heap_memory_bytes));
	printf("Telemetry: Current effective memory (should be 0): %s\n",bkbmbgbtbpbeb(telemetry.current_effective_memory_bytes));
	printf("Telemetry: The maximum bytes are utilized at the same time instant: %s\n",bkbmbgbtbpbeb(telemetry.max_heap_memory_bytes_are_utilized_at_the_same_time_instant));
	printf("Telemetry: Total heap memory free number: %s\n",form(telemetry.free_counter));
	printf("Telemetry: Total heap memory free bytes: %s\n",bkbmbgbtbpbeb(telemetry.free_bytes));
	printf("Telemetry: Total heap memory new allocations number: %s\n",form(telemetry.new_allocations_counter));
	printf("Telemetry: Total trick heap memory reallocations number: %s\n",form(telemetry.heap_trick_reallocations_counter));
	printf("Telemetry: Total true heap memory reallocations number: %s\n",form(telemetry.heap_true_reallocations_counter));
	printf("Telemetry: Total heap memory allocated during execution: %s\n",bkbmbgbtbpbeb(telemetry.total_heap_memory_allocated_bytes));
	printf("Telemetry: Total effective memory allocated during execution: %s\n",bkbmbgbtbpbeb(telemetry.total_effective_memory_allocated_bytes));
	printf("Telemetry: How many times the memory reallocation function has been called and did not returne anything: %ld\n",telemetry.how_many_times_realloc_do_nothing);
}
