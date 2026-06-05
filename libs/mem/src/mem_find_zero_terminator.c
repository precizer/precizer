#include "mem_internal.h"

/**
 * @brief Find the first zero-valued element inside a descriptor
 *
 * A zero-valued element means that every byte in that element is zero. When no
 * terminator is found within the current logical bounds, the function returns
 * @ref memory::length through @p terminator_position_out. Descriptors that
 * advertise a non-zero @ref memory::length with a `NULL`
 * @ref memory::data pointer are rejected as inconsistent. The bounded scan is
 * delegated to the shared raw string counter, which keeps the byte-sized
 * `memchr` fast path and the wider element scan in one place
 *
 * The value written through @p terminator_position_out is guaranteed only
 * when the function returns `SUCCESS`. On failure, the output object is left
 * unspecified
 *
 * @param memory_structure Descriptor to scan
 * @param terminator_position_out Output pointer for the first zero-valued
 *	element position. Its value is defined only on `SUCCESS`
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_find_zero_terminator(
	const memory *memory_structure,
	size_t       *terminator_position_out)
{
	/* This function was reviewed line by line by a human before this AI-assisted change.
	   This changed version requires a new separate human line-by-line review before it is considered trusted.
	   Any future change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(memory_structure == NULL || terminator_position_out == NULL)
	{
		report("Memory management; Invalid arguments for zero terminator search");
		status = FAILURE;
	} else if(memory_structure->single_element_size == 0){
		report("Memory management; Descriptor element size is zero (uninitialized)");
		status = FAILURE;
	} else if(memory_structure->length > 0 && memory_structure->data == NULL){
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->length == 0)
	{
		*terminator_position_out = 0;
	}

	if((TRIUMPH & status) && memory_structure->length > 0)
	{
		/* Read-only byte view over the descriptor payload used for bounded element scans */
		const unsigned char *memory_structure_data_view = (const unsigned char *)memory_structure->data;

		/* Total descriptor payload size in bytes used to guard the scan bounds */
		size_t descriptor_size_in_bytes = 0;

		run(mem_guarded_byte_size(memory_structure,memory_structure->length,&descriptor_size_in_bytes));

		if(CRITICAL & status)
		{
			report("Memory management; Zero terminator search range overflows for %zu elements of size %zu",
				memory_structure->length,
				memory_structure->single_element_size);
		}

		if(TRIUMPH & status)
		{
			run(mem_string_measure_length(
				memory_structure_data_view,
				descriptor_size_in_bytes,
				memory_structure->single_element_size,
				true,
				terminator_position_out,
				NULL));
		}
	}

	provide(status);
}
