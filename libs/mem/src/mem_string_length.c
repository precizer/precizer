#include "mem.h"

/**
 * @brief Compute visible C-string length inside a bounded descriptor.
 *
 * Scans descriptor bytes until the first `'\0'` or `memory_structure->length`,
 * whichever comes first, and writes the result to @p length_out.
 *
 * Notes:
 * - The helper does not validate `element_size`; callers should use it with
 *   byte-oriented descriptors.
 * - The function returns through `provide(status)`, so a non-`SUCCESS`
 *   `global_return_status` may override the local return code.
 */
Return memory_string_length(
	const memory *memory_structure,
	size_t       *length_out)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(memory_structure == NULL || length_out == NULL)
	{
		report("Memory management; Invalid arguments for string length helper");
		status = FAILURE;
	} else if(memory_structure->length == 0){
		*length_out = 0;
	} else if(memory_structure->data == NULL){
		*length_out = 0;
	} else {
		const unsigned char *bytes = (const unsigned char *)memory_structure->data;
		size_t index = 0;

		for(; index < memory_structure->length; ++index)
		{
			if(bytes[index] == '\0')
			{
				break;
			}
		}

		*length_out = index;
	}

	provide(status);
}
