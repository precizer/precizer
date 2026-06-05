#include "mem_internal.h"

/**
 * @brief Check whether one logical element is fully zero-valued
 *
 * The string helpers in libmem treat a terminator as one whole logical
 * element whose every byte is zero. This helper centralizes that rule so
 * callers that work with arbitrary element widths can share one fast check
 * instead of repeating byte loops in multiple files
 *
 * @param element_view Pointer to the first byte of the logical element
 * @param single_element_size Size of one logical element in bytes
 * @return `true` when every byte in the element is zero; `false` otherwise
 */
bool mem_is_zero_element(
	const unsigned char *const element_view,
	const size_t               single_element_size)
{
	if(element_view == NULL || single_element_size == 0)
	{
		return(false);
	}

	if(single_element_size == sizeof(char))
	{
		/* Fast path for one-byte elements: one zero byte already means one zero element */
		if(element_view[0] == 0)
		{
			return(true);
		}

		return(false);
	}

	for(size_t byte_index = 0; byte_index < single_element_size; ++byte_index)
	{
		if(element_view[byte_index] != 0)
		{
			return(false);
		}
	}

	return(true);
}
