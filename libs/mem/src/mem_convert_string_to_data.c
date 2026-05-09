#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Convert a descriptor from string mode to generic data mode
 *
 * Clears string metadata by setting @ref memory::string_length to `0` and
 * @ref memory::is_string to `false`. The current implementation accepts
 * byte-sized string elements only. If the descriptor currently represents a
 * canonical string view whose logical length is exactly
 * `string_length + 1` elements and whose last logical element is a zero-valued
 * terminator element, that trailing element is treated as an explicit service
 * marker for the end of the string rather than useful raw payload. The helper
 * reduces @ref memory::length by one so only the string payload remains part
 * of the logical descriptor. Descriptors that are already in data mode are
 * accepted as a successful no-op. Descriptors that advertise non-zero length
 * with a `NULL` data pointer are rejected as inconsistent
 *
 * @param memory_structure Descriptor that should from now on be treated as data
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_convert_string_to_data(memory *memory_structure)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(memory_structure == NULL)
	{
		report("Memory management; Invalid arguments for data conversion");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->length > 0 && memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->is_string == true)
	{
		if(memory_structure->single_element_size == 0)
		{
			report("Memory management; Descriptor element size is zero (uninitialized)");
			status = FAILURE;
		}

		if((TRIUMPH & status) && memory_structure->single_element_size != sizeof(char))
		{
			report("Memory management; String-to-data conversion supports byte-sized elements only");
			status = FAILURE;
		}

		if((TRIUMPH & status) &&
		   memory_structure->length > 0 &&
		   memory_structure->string_length == memory_structure->length - 1)
		{
			const unsigned char *memory_structure_data_view = (const unsigned char *)memory_structure->data;

			if(memory_structure_data_view == NULL)
			{
				report("Memory management; Data pointer is NULL while converting string to data");
				status = FAILURE;
			}

			if(TRIUMPH & status)
			{
				/* Byte offset of the last logical element, which may be the trailing terminator */
				size_t terminator_offset = 0;

				/* Becomes true only when the last logical element is an all-zero terminator */
				bool trailing_terminator_is_zero = false;

				run(mem_guarded_byte_size(memory_structure,memory_structure->length - 1,&terminator_offset));

				if(TRIUMPH & status)
				{
					trailing_terminator_is_zero = mem_is_zero_element(
						memory_structure_data_view + terminator_offset,
						memory_structure->single_element_size);
				}

				if((TRIUMPH & status) && trailing_terminator_is_zero == true)
				{
					const size_t terminator_bytes = memory_structure->single_element_size;
					memory_structure->length--;
					telemetry_current_payload_bytes_removed(terminator_bytes);
					telemetry_block_overhead_bytes_added(terminator_bytes);
				}
			}
		}
	}

	if(TRIUMPH & status)
	{
		if(memory_structure->is_string == true)
		{
			telemetry_string_to_data_conversions();
		}

		memory_structure->string_length = 0;
		memory_structure->is_string = false;
	}

	provide(status);
}
