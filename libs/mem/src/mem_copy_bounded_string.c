#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Replace destination contents with the visible part of a bounded source string
 *
 * Use this helper when the source byte range is known, but only the visible
 * prefix before the first terminator should become the new destination text.
 * The function never reads past @p source_limit_bytes and still guarantees
 * exactly one trailing terminator in the destination after replacement
 *
 * @par Active terminator search within bounds
 * Unlike @ref mem_copy_fixed_string, this function **actively scans** the
 * source within the first @p source_limit_bytes bytes looking for `'\0'`. If
 * a terminator is found earlier, only the visible prefix up to that point is
 * copied. If no terminator is found within the limit, exactly
 * @p source_limit_bytes bytes are copied. This is the safe choice when you
 * cannot guarantee that the source is terminated, but you do know an upper
 * bound for the search
 *
 * If you have absolutely no upper bound and the source is guaranteed to be
 * a real C-string, use @ref mem_copy_unbounded_string instead. If the
 * terminator is guaranteed to be exactly the last element, the faster
 * @ref mem_copy_fixed_string is preferable
 *
 * The source is interpreted using the destination element width, so the same
 * call works for ordinary `char` strings and for wider code units
 *
 * Example:
 * @code
 * const char source[] = {'n','e','w','\0','x'};
 * if((TRIUMPH & m_copy_string(path,sizeof(source),source)) == 0) { return FAILURE; }
 * @endcode
 *
 * @param destination Destination descriptor that will receive the new string.
 *        Must already be in string mode
 * @param source_limit_bytes Maximum number of bytes to scan for a terminator
 * @param source_string Source string. May or may not be terminated within bounds
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_copy_bounded_string(
	memory            *destination,
	const size_t      source_limit_bytes,
	const void *const source_string)
{
	return(mem_core_string(
		SOURCE_BOUNDED_STRING | TRANSFER_REPLACE,
		destination,
		source_limit_bytes,
		source_string));
}
