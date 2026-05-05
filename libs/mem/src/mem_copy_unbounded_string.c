#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Replace destination contents with a zero-terminated source string
 *
 * Use this helper when the source is already terminated and you want the
 * visible source payload to become the whole destination string. The source is
 * scanned until its first zero-valued terminator element, and the destination
 * ends with exactly one trailing terminator after replacement
 *
 * @par Unbounded terminator search
 * This function **actively scans** the source for `'\0'` with **no upper
 * bound**. The caller must be absolutely certain that the source is a real
 * zero-terminated C-string somewhere in memory; otherwise the scan will read
 * past the buffer end and trigger undefined behavior
 *
 * If you only have an upper bound on where the terminator could be, use
 * @ref mem_copy_bounded_string — it bounds the scan and is the safer choice.
 * If the terminator is guaranteed to be exactly the last element of a
 * fixed-size buffer, the faster @ref mem_copy_fixed_string is preferable
 *
 * The destination element width defines how the source is read, so this works
 * for byte strings and for wider string element types. Passing `NULL` means
 * "replace with an empty string"
 *
 * Example:
 * @code
 * if((TRIUMPH & m_copy_string(title,"draft")) == 0) { return FAILURE; }
 * @endcode
 *
 * @param destination Destination descriptor that will receive the new string.
 *        Must already be in string mode
 * @param source_string Zero-terminated source string, or `NULL` for an empty string
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_copy_unbounded_string(
	memory *destination,
	const void *const source_string)
{
	return(mem_core_string(
		SOURCE_UNBOUNDED_STRING | TRANSFER_REPLACE,
		destination,
		0,
		source_string));
}
