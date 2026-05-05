#include "mem.h"

/**
 * @brief Subtract one size_t value from another with underflow detection
 *
 * Used by implementation files to detect unsigned underflow when computing
 * deltas between byte counts
 *
 * @param left Left operand
 * @param right Right operand
 * @param difference Output pointer for the difference on success
 * @return Return status indicating whether the subtraction succeeded
 */
Return mem_guarded_subtract(
	size_t left,
	size_t right,
	size_t *difference)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(difference == NULL)
	{
		report("Memory management; Output pointer is NULL");
		provide(FAILURE);
	}

	if(left < right)
	{
		status = FAILURE;
		telemetry_arithmetic_guard_failure();
	}

	if(TRIUMPH & status)
	{
		*difference = left - right;
	}

	provide(status);
}
