#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Append one zero-terminated source string as a new inline element in a descriptor-backed string array
 *
 * This is the thin unbounded wrapper for @ref m_string_array_append. Use it
 * when the source is already terminated by a zero-valued element of width
 * @p single_element_size and the helper may scan until that terminator
 *
 * A `NULL` source is accepted and produces an empty appended string, matching
 * the replace semantics of @ref mem_copy_unbounded_string
 *
 * Small example:
 * @code
 * m_create(memory,string_array);
 *
 * if((TRIUMPH & m_string_array_append(string_array,char,"delta")) == 0) { return FAILURE; }
 * if((TRIUMPH & m_string_array_append(string_array,char,"epsilon")) == 0) { return FAILURE; }
 * @endcode
 *
 * @param descriptor_array Root data-mode descriptor that stores `memory` elements
 * @param single_element_size Element width in bytes for the appended string descriptor
 * @param source_text Zero-terminated source string interpreted with @p single_element_size
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_string_array_append_unbounded(
	memory            *descriptor_array,
	size_t            single_element_size,
	const void *const source_text)
{
	return(mem_string_array_core(
		SOURCE_UNBOUNDED_STRING,
		descriptor_array,
		single_element_size,
		0,
		source_text));
}
