#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Append the visible prefix of one bounded source string as a new inline element in a descriptor-backed string array
 *
 * This is the thin bounded wrapper for @ref m_string_array_append. Use it when
 * the source byte range is known in advance, but only the visible prefix before
 * the first zero-valued terminator element inside that range should be copied
 * into the appended inline descriptor
 *
 * Small example:
 * @code
 * const char source[] = {'e','p','s','i','l','o','n','\0','x'};
 * m_create(memory,string_array);
 *
 * if((TRIUMPH & m_string_array_append(string_array,char,sizeof(source),source)) == 0) { return FAILURE; }
 * @endcode
 *
 * @param descriptor_array Root data-mode descriptor that stores `memory` elements
 * @param single_element_size Element width in bytes for the appended string descriptor
 * @param source_limit_bytes Maximum byte count to inspect in @p source_text
 * @param source_text Bounded source string interpreted with @p single_element_size
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_string_array_append_bounded(
	memory *descriptor_array,
	size_t single_element_size,
	const size_t source_limit_bytes,
	const void *const source_text)
{
	return(mem_string_array_core(
		SOURCE_BOUNDED_STRING,
		descriptor_array,
		single_element_size,
		source_limit_bytes,
		source_text));
}
