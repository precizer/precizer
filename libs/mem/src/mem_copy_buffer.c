#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Copy an exact bounded byte range into a data descriptor
 *
 * This helper is the raw-buffer replace counterpart of
 * @ref mem_concat_buffer. It interprets @p source_buffer_size_bytes as the
 * exact number of source bytes to import, resizes the destination to that
 * exact payload size, and keeps the result in data mode
 *
 * Self-aliasing is supported when @p source_buffer points inside the current
 * destination allocation. Passing `NULL` together with size 0 clears the
 * destination
 *
 * @param destination Pointer to the destination descriptor receiving copied data
 * @param source_buffer_size_bytes Exact byte count to copy from @p source_buffer
 * @param source_buffer Pointer to source bytes
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_copy_buffer(
	memory            *destination,
	const size_t      source_buffer_size_bytes,
	const void *const source_buffer)
{
	return(mem_core_buffer(
		TRANSFER_REPLACE,
		destination,
		source_buffer_size_bytes,
		source_buffer));
}
