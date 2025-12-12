#include "precizer.h"

Return show_difference(
	int changes,
	const CmpctStat *source,
	const CmpctStat *destination)
{
	/* Validate input parameters */
	if(NULL == source || NULL == destination)
	{
		return(FAILURE);
	}

	if(changes == IDENTICAL)
	{
		return(SUCCESS);
	}

	const char *flags[] = {
		"size","ctime","mtime"
	};

	const int flag_values[] = {
		SIZE_CHANGED,CREATION_TIME_CHANGED,MODIFICATION_TIME_CHANGED
	};

	const int flag_count = 3;
	unsigned int flags_found = 0;
	bool first_word = true;

	/* Check each flag */
	for(int i = 0; i < flag_count; i++)
	{
		if(changes & flag_values[i])
		{
			if(first_word == true)
			{
				printf("Database file details: ");
				first_word = false;
			}

			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				printf(" & ");
			}
			printf("%s",flags[i]);
			show_metadata(i,destination,source);
			flags_found++;
		}
	}

	if(flags_found > 0)
	{
		printf("\n");
	}

	return(SUCCESS);
}
