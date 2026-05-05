#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Append the visible part of a bounded source string
 *
 * Use this helper when you know how many bytes are available in the source,
 * but you want to append only the visible text before the first terminator.
 * In other words, the function behaves like a safe bounded append: it never
 * reads past @p source_limit_bytes, and it always leaves @p destination as a
 * valid zero-terminated string
 *
 * @par Active terminator search within bounds
 * Unlike @ref mem_concat_fixed_string, this function **actively scans** the
 * source within the first @p source_limit_bytes bytes looking for `'\0'`. If
 * a terminator is found earlier, only the visible prefix up to that point is
 * appended. If no terminator is found within the limit, exactly
 * @p source_limit_bytes bytes are appended. This is the safe choice when you
 * cannot guarantee that the source is terminated, but you do know an upper
 * bound for the search
 *
 * If you have absolutely no upper bound and the source is guaranteed to be
 * a real C-string, use @ref mem_concat_unbounded_string instead. If the
 * terminator is guaranteed to be exactly the last element, the faster
 * @ref mem_concat_fixed_string is preferable
 *
 * The source is interpreted using the destination element width, so the same
 * call works for ordinary `char` strings and for wider code units
 *
 * Example:
 * @code
 * const char suffix[] = {'-','n','e','w','\0','x','x'};
 * if((TRIUMPH & mem_concat_bounded_string(path,sizeof(suffix),suffix)) == 0) { return FAILURE; }
 * // "prefix" becomes "prefix-new"
 * @endcode
 *
 * @param destination Destination string descriptor to extend.
 *        Must already be in string mode
 * @param source_limit_bytes Maximum number of bytes to scan for a terminator
 * @param source_string Source string. May or may not be terminated within bounds
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_concat_bounded_string(
	memory     *destination,
	const size_t source_limit_bytes,
	const void *const source_string)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	return(mem_core_string(
		SOURCE_BOUNDED_STRING | TRANSFER_APPEND,
		destination,
		source_limit_bytes,
		source_string));
}
