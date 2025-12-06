#include "mem.h"

Return memory_string_length(
	const memory *memory_structure,
	size_t       *length_out)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(memory_structure == NULL || length_out == NULL)
	{
		slog(ERROR,"Memory management; Invalid arguments for string length helper");
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
