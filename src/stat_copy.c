#include "precizer.h"

/**
 * @brief Copies essential elements from a `struct stat` to a `CmpctStat` structure
 *
 * This function performs a selective copy of key fields from the source `struct stat`
 * object to the destination `CmpctStat` object. The copied fields include logical
 * file size, allocated block count, device/inode identifiers, modification time
 * (seconds and nanoseconds), and status change time (seconds and nanoseconds)
 *
 * The `CmpctStat` structure is designed to contain only the minimal necessary data
 * required for comparison purposes, making it significantly more space-efficient
 * compared to the full `struct stat`. This compact representation is particularly
 * advantageous when storing large amounts of file metadata in a database,
 * where storage optimization are critical.
 *
 * @param source A pointer to the source `struct stat` object containing the original file metadata
 *               Must not be NULL. If NULL is provided, the function will fail
 * @param destination A pointer to the destination `CmpctStat` object where the selected
 *                    fields will be copied
 *
 */
Return stat_copy(
	const struct stat *source,
	CmpctStat         *destination)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(source == NULL || destination == NULL)
	{
		provide(FAILURE);
	}

	/* Copying essential elements from the stat structure to the new one */
	destination->st_size = source->st_size;
	destination->st_blocks = source->st_blocks;
	destination->st_dev = source->st_dev;
	destination->st_ino = source->st_ino;
	destination->mtim_tv_sec = source->st_mtim.tv_sec;
	destination->mtim_tv_nsec = source->st_mtim.tv_nsec;
	destination->ctim_tv_sec = source->st_ctim.tv_sec;
	destination->ctim_tv_nsec = source->st_ctim.tv_nsec;

	provide(status);
}
