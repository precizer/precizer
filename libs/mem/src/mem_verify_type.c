#include "mem.h"

Return memory_verify_type(
	const memory *memory_structure,
	size_t       expected_element_size)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(memory_structure == NULL)
	{
		slog(ERROR,"Memory management; Descriptor is NULL");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->element_size != expected_element_size)
	{
		slog(ERROR,"Memory management; Expected %zu bytes but descriptor uses %zu",expected_element_size,memory_structure->element_size);
		status = FAILURE;
	}

	provide(status);
}
