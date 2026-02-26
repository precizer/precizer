#include "mem.h"

/**
 * @brief Concatenate visible bytes from a bounded source string buffer.
 *
 * The source buffer is normalized into a temporary descriptor with
 * @ref memory_copy_cstring and then appended through @ref memory_concat_strings.
 */
Return memory_concat_cstring(
	memory     *destination,
	const char *source_buffer,
	size_t     source_buffer_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	create(char,source_view);

	if(destination == NULL)
	{
		report("Memory management; concat_cstring destination must be non-NULL");
		status = FAILURE;
	}

	run(copy_cstring(source_view,source_buffer,source_buffer_size));

	run(concat_strings(destination,source_view));

	call(del(source_view));

	provide(status);
}
