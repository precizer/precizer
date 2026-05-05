#include "test_libmem_utils.h"

Return test_libmem_0009(void)
{
	INITTEST;

	init_telemetry();

	m_create(char,buffer);

	ASSERT(telemetry.current_heap_bytes == 0);
	ASSERT(telemetry.current_payload_bytes == 0);
	ASSERT(telemetry.active_descriptors == 0);

	const size_t block = MEMORY_BLOCK_BYTES;
	const size_t big_count = block + 1;

	ASSERT(SUCCESS == m_resize(buffer,1));

	ASSERT(telemetry.current_heap_bytes == block);
	ASSERT(telemetry.current_payload_bytes == 1);
	ASSERT(telemetry.total_heap_bytes_acquired == block);
	ASSERT(telemetry.total_payload_bytes_acquired == 1);
	ASSERT(telemetry.fresh_allocations_counter == 1);
	ASSERT(telemetry.active_descriptors == 1);
	ASSERT(telemetry.peak_active_descriptors == 1);
	ASSERT(telemetry.peak_heap_bytes == block);
	ASSERT(telemetry.current_alignment_overhead_bytes == block - 1);
	ASSERT(telemetry.total_alignment_overhead_bytes == block - 1);
	ASSERT(telemetry.peak_alignment_overhead_bytes == block - 1);

	ASSERT(SUCCESS == m_resize(buffer,big_count));

	ASSERT(telemetry.current_heap_bytes == block * 2);
	ASSERT(telemetry.total_heap_bytes_acquired == block * 2);
	ASSERT(telemetry.current_payload_bytes == big_count);
	ASSERT(telemetry.total_payload_bytes_acquired == big_count);
	ASSERT(telemetry.heap_reallocations_counter == 1);
	ASSERT(telemetry.peak_heap_bytes == block * 2);

	ASSERT(SUCCESS == m_resize(buffer,1,RELEASE_UNUSED));

	ASSERT(telemetry.current_heap_bytes == block);
	ASSERT(telemetry.current_payload_bytes == 1);
	ASSERT(telemetry.total_heap_bytes_released == block);
	ASSERT(telemetry.release_unused_operations_counter == 1);
	ASSERT(telemetry.release_unused_bytes_total == block);
	ASSERT(telemetry.heap_reallocations_counter == 2);
	ASSERT(buffer->length == 1);
	ASSERT(buffer->data != NULL);

	void *const retained_data = buffer->data;
	const size_t retained_bytes = buffer->actually_allocated_bytes;

	ASSERT(SUCCESS == m_resize(buffer,0));

	ASSERT(buffer->data == retained_data);
	ASSERT(buffer->actually_allocated_bytes == retained_bytes);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(telemetry.current_heap_bytes == block);
	ASSERT(telemetry.current_payload_bytes == 0);
	ASSERT(telemetry.current_alignment_overhead_bytes == block);
	ASSERT(telemetry.total_alignment_overhead_bytes == block);
	ASSERT(telemetry.peak_alignment_overhead_bytes == block);
	ASSERT(telemetry.active_descriptors == 1);
	ASSERT(telemetry.release_operations_counter == 0);
	ASSERT(telemetry.release_unused_operations_counter == 1);
	ASSERT(telemetry.release_unused_bytes_total == block);

	ASSERT(SUCCESS == m_resize(buffer,0,RELEASE_UNUSED));

	ASSERT(buffer->data == NULL);
	ASSERT(buffer->actually_allocated_bytes == 0);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(telemetry.current_heap_bytes == 0);
	ASSERT(telemetry.current_payload_bytes == 0);
	ASSERT(telemetry.active_descriptors == 0);
	ASSERT(telemetry.release_operations_counter == 1);
	ASSERT(telemetry.release_unused_operations_counter == 2);
	ASSERT(telemetry.release_unused_bytes_total == block * 2);
	ASSERT(telemetry.total_heap_bytes_released == block * 2);
	ASSERT(telemetry.current_alignment_overhead_bytes == 0);

	ASSERT(SUCCESS == m_del(buffer));

	ASSERT(telemetry.current_heap_bytes == 0);
	ASSERT(telemetry.current_payload_bytes == 0);
	ASSERT(telemetry.active_descriptors == 0);
	ASSERT(telemetry.release_operations_counter == 1);
	ASSERT(telemetry.total_heap_bytes_released == block * 2);
	ASSERT(telemetry.peak_heap_bytes == block * 2);
	ASSERT(telemetry.current_alignment_overhead_bytes == 0);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(buffer->data == NULL);
	ASSERT(buffer->actually_allocated_bytes == 0);

	#if SHOW_TEST
	telemetry_show();
	#endif

	RETURN_STATUS;
}
