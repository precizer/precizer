#include "precizer.h"

/**
 * @brief Retrieve a const pointer to a flag descriptor by index.
 *
 * Uses the mem helper to obtain a typed readonly view of the @ref Flags array and
 * performs bounds checking. Returns NULL if the descriptor is missing, type
 * verification fails, or the index is out of range.
 */
static const Flags *lookup(
	const memory *flags,
	size_t        index)
{
	const Flags *flags_data = cdata(Flags,flags);

	if(flags_data == NULL || index >= flags->length)
	{
		return(NULL);
	}

	return(&flags_data[index]);
}

Return show_difference(
	Changed         change_flags_mask,
	const CmpctStat *before,
	const CmpctStat *after)
{
	/* Validate input parameters */
	if(NULL == before || NULL == after)
	{
		return(FAILURE);
	}

	if(change_flags_mask == IDENTICAL)
	{
		return(SUCCESS);
	}

	Return status = SUCCESS;

	create(Flags,flags);
	call(resize(flags,3));

	Flags *flags_data = data(Flags,flags);

	if(flags_data == NULL)
	{
		del(flags);
		provide(FAILURE);
	}

	flags_data[0] = (Flags){SIZE_CHANGED,"size"};
	flags_data[1] = (Flags){STATUS_CHANGED_TIME,"ctime"};
	flags_data[2] = (Flags){MODIFICATION_TIME_CHANGED,"mtime"};

	unsigned int flags_found = 0;
	bool first_word = true;

	/* Check each flag */
	for(size_t i = 0; i < flags->length; i++)
	{
		const Flags *flag = lookup(flags,i);

		if(flag == NULL)
		{
			break;
		}

		if(((unsigned int)change_flags_mask & (unsigned int)flag->flag_value) != 0u)
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
			printf("%s",flag->flag_name);
			show_metadata(flag->flag_value,before,after);
			flags_found++;
		}
	}

	del(flags);

	if(flags_found > 0)
	{
		printf("\n");
	}

	return(SUCCESS);
}
