#include "rational.h"

/**
 * @brief Normalize one function Return status with global context
 *
 * @details The function normalizes @p status, normalizes global_return_status,
 *          stores the normalized global status back, merges only GLOBAL bits
 *          from global_return_status into @p status, and normalizes the final
 *          result. Technical critical bits take precedence over SUCCESS.
 *          NO takes precedence over YES when both binary answer flags are present
 *
 * @param status Status flags to normalize
 * @return Normalized status flags
 */
Return normalize_return_status(Return status)
{
	/* CRITICAL marks an internal or blocking problem.
	   Such a status cannot also be SUCCESS, even if SUCCESS was ORed in earlier */
	if(CRITICAL & status)
	{
		status &= ~SUCCESS;
	}

	/* NO is the dominant binary answer.
	   If a check answered no, the final binary answer cannot also be yes.
	   If both binary answers are present, keep NO and drop YES */
	if(NO & status)
	{
		status &= ~YES;
	}

	/* Local status is not the only source of process state.
	   global_return_status carries context that may be set outside the current function.
	   Read it into a regular Return value, normalize that copy, store the cleaned copy back,
	   then merge only explicitly global bits into the function return */
	Return global_status = atomic_load(&global_return_status);

	/* Global status follows the same contradiction rules as local status.
	   The normalized value is stored back so future returns see cleaned flags */
	if(CRITICAL & global_status)
	{
		global_status &= ~SUCCESS;
	}

	if(NO & global_status)
	{
		global_status &= ~YES;
	}

	/* Keep the process-wide status normalized for the next function return */
	atomic_store(&global_return_status,global_status);

	/* Only process-wide context bits may flow from global status to a return.
	   Binary answers and local-only flags must not leak between functions */
	status |= (global_status & GLOBAL);

	/* Merging global context can introduce contradictions.
	   Normalize once more before returning the final function status */
	if(CRITICAL & status)
	{
		status &= ~SUCCESS;
	}

	if(NO & status)
	{
		status &= ~YES;
	}

	return(status);
}

/// Converts Return status flags to a string
const char *show_status(const Return status)
{
	const char *status_label = "UNKNOWN";

	/* Keep behavior explicit for the zero status value. */
	if(status == OK)
	{
		status_label = "OK";
	} else {
		static char buffer[MAX_CHARACTERS];
		buffer[0] = '\0';

		static const struct {
			Return flag;
			const char *name;
		} mapping[] = {
			{FAILURE,"FAILURE"},
			{SUCCESS,"SUCCESS"},
			{HALTED,"HALTED"},
			{WARNING,"WARNING"},
			{DONOTHING,"DONOTHING"},
			{INFO,"INFO"},
			{YES,"YES"},
			{NO,"NO"},
			{0,NULL}
		};

		size_t used = 0U;
		bool first = true;
		unsigned int remaining = (unsigned int)status;

		for(size_t i = 0U; mapping[i].name != NULL; i++)
		{
			const unsigned int flag = (unsigned int)mapping[i].flag;

			if((remaining & flag) == 0U)
			{
				continue;
			}

			const int written = snprintf(
				buffer + used,
				MAX_CHARACTERS - used,
				"%s%s",
				first ? "" : "|",
				mapping[i].name);

			if(written < 0)
			{
				break;
			}

			if((size_t)written >= (MAX_CHARACTERS - used))
			{
				buffer[MAX_CHARACTERS - 1U] = '\0';
				status_label = buffer;
				first = false;
				break;
			}

			used += (size_t)written;
			first = false;
			remaining &= ~flag;
		}

		if(first == false)
		{
			status_label = buffer;
		}
	}
	return(status_label);
}
