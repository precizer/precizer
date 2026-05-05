#include "mem_internal.h"
#include <string.h>

/**
 * @brief Count visible elements in a raw zero-terminated string
 *
 * This internal helper measures a raw source string without requiring a
 * `memory` descriptor. It supports two scan modes. In bounded mode, it inspects
 * at most @p source_limit_bytes bytes and reports either the first zero-valued
 * element position or the number of whole elements that fit into that byte
 * range. In unbounded mode, it scans until the first zero-valued element
 *
 * A zero-valued element means that every byte in that element is zero. For
 * byte-sized elements the helper delegates to libc `memchr` in bounded mode and
 * `strlen` in unbounded mode. Wider elements are scanned element by element so
 * a zero byte inside a wider element does not accidentally terminate the string
 *
 * @warning Unbounded mode has the same safety contract as `strlen`. The caller
 * must guarantee that @p source_string is non-NULL and truly terminated by a
 * zero-valued element of width @p single_element_size
 *
 * @param source_string Raw source bytes to inspect
 * @param source_limit_bytes Maximum byte range to inspect in bounded mode
 * @param single_element_size Size of one logical string element in bytes
 * @param source_limit_is_active Whether @p source_limit_bytes limits the scan
 * @param length_out Output pointer receiving the visible element count
 * @param terminator_found_out Optional output pointer receiving whether a
 *        zero-valued terminator was found
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_string_measure_length(
	const void *const source_string,
	const size_t source_limit_bytes,
	const size_t single_element_size,
	const bool source_limit_is_active,
	size_t *const length_out,
	bool *const terminator_found_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Visible source prefix length measured in elements */
	size_t measured_length = 0;

	/* Becomes true once a zero-valued terminator element is found */
	bool terminator_found = false;

	if(source_string == NULL || length_out == NULL)
	{
		report("Memory management; Invalid arguments for raw string length counter");
		status = FAILURE;
	}

	if((TRIUMPH & status) && single_element_size == 0)
	{
		report("Memory management; Raw string element size is zero");
		status = FAILURE;
	}

	if((TRIUMPH & status) && single_element_size == sizeof(char))
	{
		const unsigned char *const source_data_view = (const unsigned char *)source_string;

		if(source_limit_is_active == true)
		{
			/* Address of the first zero byte found by the byte-sized fast path */
			const void *found_terminator = memchr(source_data_view,0,source_limit_bytes);

			if(found_terminator == NULL)
			{
				measured_length = source_limit_bytes;
			} else {
				measured_length = (size_t)((const unsigned char *)found_terminator - source_data_view);
				terminator_found = true;
			}
		} else {
			measured_length = strlen((const char *)source_string);
			terminator_found = true;
		}
	}

	if((TRIUMPH & status) && single_element_size != sizeof(char))
	{
		const unsigned char *source_data_view = (const unsigned char *)source_string;

		if(source_limit_is_active == true)
		{
			/* Byte offset of the current logical element being inspected */
			size_t element_offset = 0;

			while(element_offset + single_element_size <= source_limit_bytes)
			{
				if(mem_is_zero_element(source_data_view + element_offset,single_element_size) == true)
				{
					terminator_found = true;
					break;
				}

				++measured_length;
				element_offset += single_element_size;
			}
		} else {
			while(mem_is_zero_element(source_data_view,single_element_size) == false)
			{
				source_data_view += single_element_size;
				++measured_length;
			}

			terminator_found = true;
		}
	}

	if(TRIUMPH & status)
	{
		*length_out = measured_length;

		if(terminator_found_out != NULL)
		{
			*terminator_found_out = terminator_found;
		}
	}

	provide(status);
}
