#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Append exact bytes from a bounded source buffer to a data descriptor
 *
 * This public append helper is a thin wrapper around @ref mem_core_buffer in
 * append mode. The exact-byte contract, self-aliasing rules, and validation of
 * data-mode destinations are all handled by the shared internal core
 *
 * @param destination Pointer to the destination descriptor receiving appended data
 * @param source_buffer_size_bytes Total bytes available in @p source_buffer
 * @param source_buffer Pointer to source bytes
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_concat_buffer(
	memory            *destination,
	const size_t      source_buffer_size_bytes,
	const void *const source_buffer)
{
	return(mem_core_buffer(
		TRANSFER_APPEND,
		destination,
		source_buffer_size_bytes,
		source_buffer));
}
