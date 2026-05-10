#include "mem_internal.h"
#include <string.h>

/**
 * @brief Overwrite one logical element with a zero terminator
 *
 * Writes a zero-valued element at @p terminator_index using the descriptor's
 * configured element width. The helper performs only the minimal descriptor
 * checks needed to reject impossible writes while remaining reusable across
 * string helpers inside the library
 *
 * @param memory_structure Descriptor whose buffer receives the terminator
 * @param terminator_index Logical element index where the terminator is written
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_write_zero_terminator(
	memory *memory_structure,
	const size_t terminator_index)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	size_t terminator_offset = 0;
	size_t required_bytes = 0;

	if(memory_structure == NULL || memory_structure->single_element_size == 0)
	{
		report("Memory management; Invalid descriptor for zero terminator write");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->data == NULL)
	{
		report("Memory management; Data pointer is NULL during zero terminator write");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		run(mem_guarded_byte_size(memory_structure,terminator_index,&terminator_offset));
		run(mem_guarded_add(terminator_offset,memory_structure->single_element_size,&required_bytes));

		if(CRITICAL & status)
		{
			report("Memory management; Zero terminator write overflows descriptor bounds");
		}
	}

	if((TRIUMPH & status) && required_bytes > memory_structure->actually_allocated_bytes)
	{
		report("Memory management; Zero terminator write exceeds reserved capacity");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		memset((unsigned char *)memory_structure->data + terminator_offset,0,memory_structure->single_element_size);
		telemetry_string_terminator_writes();
	}

	provide(status);
}
