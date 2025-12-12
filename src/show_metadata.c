#include "precizer.h"

/**
 * @brief Prints metadata details based on the specified flag.
 *
 * This function outputs metadata such as file size or timestamps (creation and modification times)
 * in a human-readable format. It is used to display changes between two sets of file metadata.
 *
 * @param[in] flag Identifier for the type of metadata to print:
 *                 - 0: File size
 *                 - 1: Creation time
 *                 - 2: Modification time
 */
void show_metadata(
	int             flag,
	const CmpctStat *was,
	const CmpctStat *now)
{
	if(rational_logger_mode & (VERBOSE|TESTING))
	{
		switch(flag)
		{
			case 0:
				printf(" was:%s",bkbmbgbtbpbeb((size_t)was->st_size));
				printf(", now:%s",bkbmbgbtbpbeb((size_t)now->st_size));
				break;
			case 1:
				printf(" was:%s.%ld",seconds_to_ISOdate(was->ctim_tv_sec),was->ctim_tv_nsec);
				printf(", now:%s.%ld",seconds_to_ISOdate(now->ctim_tv_sec),now->ctim_tv_nsec);
				break;
			case 2:
				printf(" was:%s.%ld",seconds_to_ISOdate(was->mtim_tv_sec),was->mtim_tv_nsec);
				printf(", now:%s.%ld",seconds_to_ISOdate(now->mtim_tv_sec),now->mtim_tv_nsec);
				break;
			default:
				return;
				break;
		}
	}
}
