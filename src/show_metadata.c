#include "precizer.h"

/**
 * @brief Prints metadata details based on the specified flag.
 *
 * This function outputs metadata such as file size or timestamps (creation and modification times)
 * in a human-readable format. It is used to display changes between two sets of file metadata.
 *
 * @param[in] flag Identifier for the type of metadata to print.
 */
void show_metadata(
	Changed         flag,
	const CmpctStat *was,
	const CmpctStat *now)
{
	switch(flag)
	{
		case SIZE_CHANGED:
			slog(EVERY|UNDECOR," was:%s",bkbmbgbtbpbeb((size_t)was->st_size));
			slog(EVERY|UNDECOR,", now:%s",bkbmbgbtbpbeb((size_t)now->st_size));
			break;
		case STATUS_CHANGED_TIME:
			slog(EVERY|UNDECOR," was:%s.%ld",seconds_to_ISOdate(was->ctim_tv_sec),was->ctim_tv_nsec);
			slog(EVERY|UNDECOR,", now:%s.%ld",seconds_to_ISOdate(now->ctim_tv_sec),now->ctim_tv_nsec);
			break;
		case MODIFICATION_TIME_CHANGED:
			slog(EVERY|UNDECOR," was:%s.%ld",seconds_to_ISOdate(was->mtim_tv_sec),was->mtim_tv_nsec);
			slog(EVERY|UNDECOR,", now:%s.%ld",seconds_to_ISOdate(now->mtim_tv_sec),now->mtim_tv_nsec);
			break;
		default:
			return;
			break;
	}
}
