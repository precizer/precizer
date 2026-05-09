#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Free the allocated block and clear the descriptor lengths
 *
 * @param memory_structure Pointer to a descriptor
 *
 * @post For descriptors that pass validation, sets @ref memory::data to `NULL`,
 *       @ref memory::length to `0`, @ref memory::actually_allocated_bytes to `0`,
 *       and @ref memory::string_length to `0`. The current value of
 *       @ref memory::is_string is preserved, and
 *       @ref memory::single_element_size remains unchanged so the descriptor can be
 *       allocated again for the same element type in the same mode
 *
 * @note Descriptors that advertise non-zero length with a `NULL` data pointer
 *       are reported as invalid and left unchanged. In that case the function
 *       returns `FAILURE`
 */
Return mem_delete(memory *memory_structure)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(memory_structure == NULL)
	{
		report("Memory management; Descriptor is NULL");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->length > 0 && memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		const size_t previously_allocated = memory_structure->actually_allocated_bytes;
		size_t previous_payload_bytes = 0;
		size_t previous_block_overhead = 0;

		run(mem_guarded_byte_size(memory_structure,memory_structure->length,&previous_payload_bytes));

		if(previously_allocated > previous_payload_bytes)
		{
			/* Direct subtraction is safe because the if-guard above proves no underflow */
			previous_block_overhead = previously_allocated - previous_payload_bytes;
		}

		if(memory_structure->data != NULL)
		{
			free(memory_structure->data);

			if(previously_allocated > 0)
			{
				telemetry_current_heap_reserved_bytes_released(previously_allocated);
				telemetry_total_heap_reserved_bytes_released(previously_allocated);
			}

			telemetry_heap_buffer_releases();
			telemetry_active_descriptors_released();
		}

		if(previous_payload_bytes > 0)
		{
			telemetry_current_payload_bytes_removed(previous_payload_bytes);
		}

		if(previous_block_overhead > 0)
		{
			telemetry_current_block_overhead_bytes_removed(previous_block_overhead);
		}

		memory_structure->data = NULL;
		memory_structure->length = 0;
		memory_structure->actually_allocated_bytes = 0;
		memory_structure->string_length = 0;
	}

	provide(status);
}
