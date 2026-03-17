#include "precizer.h"

/**
 * @brief Convert POSIX `st_blocks` units into bytes.
 *
 * @details POSIX defines `st_blocks` in 512-byte units regardless of the
 * filesystem I/O block size. Some filesystems may report non-positive values
 * for special files; those are normalized to zero.
 *
 * @param blocks Allocated block count from file metadata.
 * @return Allocated bytes as `blocks * POSIX_STAT_BLOCK_BYTES`, or zero when
 *         @p blocks is less than or equal to zero.
 */
extern inline size_t blocks_to_bytes(const blkcnt_t blocks)
{
	if(blocks <= 0)
	{
		return 0;
	}

	// Guard against multiplication overflow: if blocks exceeds the safe range,
	// return SIZE_MAX as a saturating upper bound instead of wrapping around
	if((size_t)blocks > SIZE_MAX / POSIX_STAT_BLOCK_BYTES)
	{
		return SIZE_MAX;
	}

	return (size_t)blocks * POSIX_STAT_BLOCK_BYTES;
}
