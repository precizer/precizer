#include "mem.h"

/**
 * @brief Compute a descriptor-backed byte size with overflow detection
 *
 * Uses @ref memory::single_element_size from @p memory_structure and multiplies it by
 * @p element_count after validating the descriptor metadata
 *
 * @param memory_structure Descriptor providing the element size
 * @param element_count Number of elements to convert into bytes
 * @param size_in_bytes Output pointer for the byte size on success
 * @return Return status indicating whether the size calculation succeeded
 */
Return mem_guarded_byte_size(
	const memory *memory_structure,
	size_t element_count,
	size_t *size_in_bytes)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(size_in_bytes == NULL)
	{
		report("Memory management; Output pointer is NULL");
		provide(FAILURE);
	}

	if(memory_structure == NULL)
	{
		report("Memory management; Descriptor is NULL");
		provide(FAILURE);
	}

	if(memory_structure->single_element_size == 0)
	{
		report("Memory management; Descriptor element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(element_count > SIZE_MAX / memory_structure->single_element_size)
	{
		status = FAILURE;
		telemetry_arithmetic_guard_failures();
	}

	if(TRIUMPH & status)
	{
		*size_in_bytes = memory_structure->single_element_size * element_count;
	}

	provide(status);
}
