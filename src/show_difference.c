#include "precizer.h"

/**
 * @brief Retrieve a const pointer to a flag descriptor by index
 *
 * Uses the mem helper to obtain a typed readonly view of the @ref Flags array and
 * performs bounds checking. Returns NULL if the descriptor is missing, type
 * verification fails, or the index is out of range
 */
static const Flags *lookup(
	const memory *flags,
	size_t       index)
{
	const Flags *flags_data_view = m_data_ro(Flags,flags);

	if(flags_data_view == NULL || index >= flags->length)
	{
		return(NULL);
	}

	return(&flags_data_view[index]);
}

/**
 * @brief Print database metadata fields that changed between two snapshots
 *
 * Expands a metadata-change bit mask into user-visible field names and detailed
 * values. The function is used when final database-file consistency checks need
 * to explain why the database file metadata differs from the saved baseline
 *
 * @param[in] change_flags_mask Bit mask describing which metadata fields changed
 * @param[in] before Saved metadata snapshot from the beginning of the run
 * @param[in] after Current metadata snapshot from the end of the run
 * @return SUCCESS when there is nothing to print or the difference was printed,
 *         otherwise FAILURE for missing snapshots
 */
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

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(Flags,flags);
	call(m_resize(flags,4));

	Flags *flags_data_rewritable = m_data(Flags,flags);

	if(flags_data_rewritable == NULL)
	{
		m_del(flags);
		provide(FAILURE);
	}

	flags_data_rewritable[0] = (Flags){SIZE_CHANGED,"lsize"};
	flags_data_rewritable[1] = (Flags){ALLOCATED_SIZE_CHANGED,"asize"};
	flags_data_rewritable[2] = (Flags){STATUS_CHANGED_TIME,"ctime"};
	flags_data_rewritable[3] = (Flags){MODIFICATION_TIME_CHANGED,"mtime"};

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
				slog(ERROR,"Database file details: ");
				first_word = false;
			}

			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				slog(ERROR|UNDECOR," & ");
			}
			slog(ERROR|UNDECOR,"%s",flag->flag_name);
			show_metadata(ERROR,flag->flag_value,before,after);
			flags_found++;
		}
	}

	if(flags_found > 0)
	{
		slog(ERROR|UNDECOR,"\n");
	}

	m_del(flags);

	provide(SUCCESS);
}
