#include "precizer.h"

#include <errno.h>
#include <stdint.h>

/**
 * @brief Pause execution at a selected test wait point.
 *
 * The function reads `TESTITALL_SIGNAL_WAIT_POINT` and
 * `TESTITALL_SIGNAL_WAIT_MS`. When the configured point matches @p point_id,
 * it waits for the requested number of milliseconds, checking
 * `global_interrupt_flag` between short sleep chunks for early resume.
 */
void signal_wait_at_point(unsigned int point_id)
{
	const char *configured_point = getenv("TESTITALL_SIGNAL_WAIT_POINT");

	if(NULL == configured_point || '\0' == configured_point[0])
	{
		return;
	}

	errno = 0;
	char *point_end_ptr = NULL;
	unsigned long long parsed_point_id = strtoull(configured_point,&point_end_ptr,10);

	if(errno != 0 || point_end_ptr == configured_point || '\0' != *point_end_ptr)
	{
		return;
	}

	if(parsed_point_id != (unsigned long long)point_id)
	{
		return;
	}

	const char *timeout_text = getenv("TESTITALL_SIGNAL_WAIT_MS");

	if(NULL == timeout_text || '\0' == timeout_text[0])
	{
		return;
	}

	errno = 0;
	char *end_ptr = NULL;
	unsigned long long parsed_timeout_ms = strtoull(timeout_text,&end_ptr,10);

	if(errno != 0 || end_ptr == timeout_text || '\0' != *end_ptr || parsed_timeout_ms == 0ULL)
	{
		return;
	}

	uint64_t remaining_timeout_ms = (uint64_t)parsed_timeout_ms;

	while(remaining_timeout_ms > 0U)
	{
		/* Allow tests to release the delay as soon as the signal handler sets the flag. */
		if(atomic_load(&global_interrupt_flag) == true)
		{
			return;
		}

		uint64_t chunk_ms = remaining_timeout_ms;
		if(chunk_ms > 10U)
		{
			chunk_ms = 10U;
		}

		struct timespec delay = {
			.tv_sec = 0,
			.tv_nsec = (long)(chunk_ms * 1000000ULL)
		};

		while(nanosleep(&delay,&delay) == -1 && errno == EINTR)
		{
			if(atomic_load(&global_interrupt_flag) == true)
			{
				return;
			}
		}

		remaining_timeout_ms -= chunk_ms;
	}
}
