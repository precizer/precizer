#include "mem.h"

/**
 * @brief Add two size_t values with overflow detection
 *
 * Used by implementation files to detect overflows when computing element
 * counts or string sizes
 *
 * @param left Left operand
 * @param right Right operand
 * @param sum Output pointer for the sum on success
 * @return Return status indicating whether the addition succeeded
 */
Return mem_guarded_add(
	size_t left,
	size_t right,
	size_t *sum)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(sum == NULL)
	{
		report("Memory management; Output pointer is NULL");
		provide(FAILURE);
	}

	if(left > SIZE_MAX - right)
	{
		status = FAILURE;
		telemetry_arithmetic_guard_failure();
	}

	if(TRIUMPH & status)
	{
		*sum = left + right;
	}

	provide(status);
}
