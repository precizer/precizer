#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Replace destination contents with a fixed-size string source whose final element is the terminator
 *
 * Use this helper when the full source size is already known and the last
 * logical element inside that size is the terminator. The helper does not need
 * `strlen(...)` or any other terminator search because fixed-string mode accepts
 * the final element as the terminator by contract
 *
 * @par Trust mode — no terminator search
 * This is the "trust mode" variant: the source is **not scanned** for an
 * embedded `'\0'`. The function simply trusts that the last element within
 * @p source_size_bytes is the terminator and copies the entire range as-is. If you
 * are not certain that the terminator is exactly at the end of the source
 * buffer, do not use this function — use @ref mem_copy_bounded_string or
 * @ref mem_copy_unbounded_string instead, which actively search for the
 * terminator
 *
 * This works for ordinary byte strings and for wider fixed-size string arrays
 * as long as the destination element width matches the source element width
 *
 * @par Convenience for string literals
 * For C string literals, prefer the @ref m_copy_literal macro, which derives
 * the size automatically via `sizeof` and avoids manual size arithmetic
 *
 * Example:
 * @code
 * const char title_text[] = {'d','r','a','f','t','\0'};
 * if((TRIUMPH & m_copy_fixed_string(title,sizeof(title_text),title_text)) == 0) { return FAILURE; }
 * @endcode
 *
 * @param destination Destination descriptor that will receive the new source.
 *        Must already be in string mode
 * @param source_size_bytes Total source size in bytes, including the terminator
 * @param source Source string whose last logical element is the terminator
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_copy_fixed_string(
	memory            *destination,
	const size_t      source_size_bytes,
	const void *const source)
{
	return(mem_core_string(
		SOURCE_FIXED_STRING | TRANSFER_REPLACE,
		destination,
		source_size_bytes,
		source));
}
