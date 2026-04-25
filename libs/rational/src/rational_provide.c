#include "rational.h"

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
			{YES,"YES"},
			{UNSUCCESS,"UNSUCCESS"},
			{SUCCESS,"SUCCESS"},
			{HALTED,"HALTED"},
			{WARNING,"WARNING"},
			{DONOTHING,"DONOTHING"},
			{INFO,"INFO"},
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
