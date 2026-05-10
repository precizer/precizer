#include "test_libmem_utils.h"

/**
 * @brief Verify that a process-wide global_return_status of INFO flows
 *        through libmem resizes without skipping internal run() bookkeeping
 *
 * Sets global_return_status to INFO, performs an m_resize-driven shrink
 * of a string descriptor, then restores the global status. The test
 * asserts that the resize return carries TRIUMPH and INFO, that HALTED
 * does not leak in, and that the descriptor actually shrank to the new
 * visible length. The Telemetry struct is used as a drift detector: if
 * any internal run(...) step inside m_resize had been silently skipped
 * because of the INFO flag, the current_* counters would not return to
 * baseline after m_del. So this test is about how a process-wide global
 * status propagates through libmem operations, not about telemetry
 * itself
 *
 * @return Return describing success or failure
 */
Return test_libmem_0071(void)
{
	INITTEST;

	const size_t baseline_heap_bytes = telemetry.current_heap_reserved_bytes;
	const size_t baseline_payload_bytes = telemetry.current_payload_bytes;
	const size_t baseline_block_overhead_bytes = telemetry.current_block_overhead_bytes;
	const size_t baseline_current_active_descriptors = telemetry.current_active_descriptors;

	const size_t initial_elements = sizeof("graceful////");
	const size_t shrunken_elements = sizeof("graceful");
	const size_t expected_heap_drift = MEMORY_BLOCK_BYTES;

	m_create(char,graceful_path,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(graceful_path,"graceful////"));
	ASSERT(telemetry.current_heap_reserved_bytes == baseline_heap_bytes + expected_heap_drift);
	ASSERT(telemetry.current_payload_bytes == baseline_payload_bytes + initial_elements);
	ASSERT(telemetry.current_block_overhead_bytes ==
		baseline_block_overhead_bytes + expected_heap_drift - initial_elements);
	ASSERT(telemetry.current_active_descriptors == baseline_current_active_descriptors + 1U);

	if(SUCCESS == status)
	{
		Return saved_global_status = atomic_exchange(&global_return_status,INFO);
		Return resize_status = m_resize(graceful_path,shrunken_elements);
		atomic_store(&global_return_status,saved_global_status);

		ASSERT(TRIUMPH & resize_status);
		ASSERT(INFO & resize_status);
		ASSERT((HALTED & resize_status) == 0);
	}

	ASSERT(graceful_path->length == shrunken_elements);
	ASSERT(graceful_path->string_length == shrunken_elements - 1U);
	ASSERT(0 == strcmp(m_text(graceful_path),"graceful"));
	ASSERT(telemetry.current_heap_reserved_bytes == baseline_heap_bytes + expected_heap_drift);
	ASSERT(telemetry.current_payload_bytes == baseline_payload_bytes + shrunken_elements);
	ASSERT(telemetry.current_block_overhead_bytes ==
		baseline_block_overhead_bytes + expected_heap_drift - shrunken_elements);
	ASSERT(telemetry.current_active_descriptors == baseline_current_active_descriptors + 1U);

	call(m_del(graceful_path));

	ASSERT(telemetry.current_heap_reserved_bytes == baseline_heap_bytes);
	ASSERT(telemetry.current_payload_bytes == baseline_payload_bytes);
	ASSERT(telemetry.current_block_overhead_bytes == baseline_block_overhead_bytes);
	ASSERT(telemetry.current_active_descriptors == baseline_current_active_descriptors);

	RETURN_STATUS;
}
