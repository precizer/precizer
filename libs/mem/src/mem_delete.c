#include "mem.h"

Return memory_delete(memory *memory_structure)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(memory_structure == NULL)
	{
		report("Memory management; Descriptor is NULL");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		const size_t previously_allocated = memory_structure->actually_allocated_bytes;
		size_t previous_effective_bytes = 0;
		size_t previous_alignment_overhead = 0;

		run(memory_guarded_size(memory_structure->element_size,
			memory_structure->length,
			&previous_effective_bytes));

		if(previously_allocated > previous_effective_bytes)
		{
			previous_alignment_overhead = previously_allocated - previous_effective_bytes;
		}

		if(memory_structure->data != NULL)
		{
			free(memory_structure->data);

			if(previously_allocated > 0)
			{
				telemetry_reduce(previously_allocated);
				telemetry_free_total_bytes(previously_allocated);
			}

			telemetry_free_counter();
			telemetry_active_descriptor_release();
		}

		if(previous_effective_bytes > 0)
		{
			telemetry_effective_reduce(previous_effective_bytes);
		}

		if(previous_alignment_overhead > 0)
		{
			telemetry_alignment_overhead_reduce(previous_alignment_overhead);
		}

		memory_structure->data = NULL;
		memory_structure->length = 0;
		memory_structure->actually_allocated_bytes = 0;
	}

	provide(status);
}
