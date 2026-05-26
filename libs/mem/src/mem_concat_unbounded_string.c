#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Append a zero-terminated source string without a separate size limit
 *
 * Use this helper when the source really is terminated and you want familiar
 * `strcat`-style behavior for this library's typed string descriptors. The
 * function scans the source up to its first zero-valued terminator, appends
 * only the visible payload, and leaves @p destination with exactly one
 * trailing terminator
 *
 * @par Unbounded terminator search
 * This function **actively scans** the source for `'\0'` with **no upper
 * bound**. The caller must be absolutely certain that the source is a real
 * zero-terminated C-string somewhere in memory; otherwise the scan will read
 * past the buffer end and trigger undefined behavior
 *
 * If you only have an upper bound on where the terminator could be, use
 * @ref mem_concat_bounded_string — it bounds the scan and is the safer
 * choice. If the terminator is guaranteed to be exactly the last element of
 * a fixed-size buffer, the faster @ref mem_concat_fixed_string is preferable
 *
 * The destination element width defines how the source is read, so the helper
 * works for `char`, `wchar_t`, `char16_t`, `char32_t`, `uint16_t`, `uint32_t`,
 * and similar element types. Passing `NULL` as @p source_string means "append
 * nothing"
 *
 * Example:
 * @code
 * if((TRIUMPH & mem_concat_unbounded_string(title," draft")) == 0) { return FAILURE; }
 * // "Report" becomes "Report draft"
 * @endcode
 *
 * @param destination Destination string descriptor to extend.
 *        Must already be in string mode
 * @param source_string Zero-terminated source string, or `NULL` for an empty append
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_concat_unbounded_string(
	memory            *destination,
	const void *const source_string)
{
	return(mem_core_string(
		SOURCE_UNBOUNDED_STRING | TRANSFER_APPEND,
		destination,
		0,
		source_string));
}
