#include "precizer.h"

/**
 * @brief Prints metadata details based on the specified flag.
 *
 * This function outputs metadata such as file size or timestamps (creation and modification times)
 * in a human-readable format. It is used to display changes between two sets of file metadata.
 *
 * @param level Logger level to use (e.g., ERROR for error paths, EVERY for regular output)
 * @param[in] flag Identifier for the type of metadata to print.
 */
void show_metadata(
	LOGMODES        level,
	Changed         flag,
	const CmpctStat *was,
	const CmpctStat *now)
{
	const unsigned int log_level = (unsigned int)(level | UNDECOR);

	switch(flag)
	{
		case SIZE_CHANGED:
			{
				slog(log_level," was:%s",bkbmbgbtbpbeb((size_t)was->st_size,FULL_VIEW));
				slog(log_level,", now:%s",bkbmbgbtbpbeb((size_t)now->st_size,FULL_VIEW));
			}
			break;
		case ALLOCATED_SIZE_CHANGED:
			{
				/* This legacy can be removed in 2036 (10-year Long-Term Support) */
				if(was->st_blocks == BLKCNT_UNKNOWN)
				{
					break;
				}
				slog(log_level," was:%s",bkbmbgbtbpbeb(blocks_to_bytes(was->st_blocks),FULL_VIEW));
				slog(log_level,", now:%s",bkbmbgbtbpbeb(blocks_to_bytes(now->st_blocks),FULL_VIEW));
			}
			break;
		case STATUS_CHANGED_TIME:
			{
				slog(log_level," was:%s.%ld",seconds_to_ISOdate(was->ctim_tv_sec),was->ctim_tv_nsec);
				slog(log_level,", now:%s.%ld",seconds_to_ISOdate(now->ctim_tv_sec),now->ctim_tv_nsec);
			}
			break;
		case MODIFICATION_TIME_CHANGED:
			{
				slog(log_level," was:%s.%ld",seconds_to_ISOdate(was->mtim_tv_sec),was->mtim_tv_nsec);
			  slog(log_level,", now:%s.%ld",seconds_to_ISOdate(now->mtim_tv_sec),now->mtim_tv_nsec);
			}
			break;
		default:
			return;
			break;
	}
}
