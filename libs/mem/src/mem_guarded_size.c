#include "mem.h"

Return memory_guarded_size(
	size_t left,
	size_t right,
	size_t *product)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(left != 0 && right > SIZE_MAX / left)
	{
		status = FAILURE;
		telemetry_overflow_guard_failure();
	}

	if(SUCCESS == status)
	{
		*product = left * right;
	}

	provide(status);
}
