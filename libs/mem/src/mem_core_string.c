#include "mem.h"
#include "mem_internal.h"
#include <stdint.h>

/**
 * @brief Internal string transfer helper used by both append and copy paths
 *
 * This function is the shared core behind the public string helpers. It knows
 * how to read a source string in several ways and then either append that
 * visible payload to the destination or replace the destination with it
 *
 * The destination must already be a string descriptor in both append and
 * replace modes. Callers that start from a data-mode buffer must convert it
 * first via @ref mem_convert_data_to_string. This function never promotes a
 * data-mode descriptor into string mode by itself
 *
 * Think about the behavior as two independent switches:
 * - source mode answers "how should the source length be understood"
 * - transfer mode answers "should that visible payload be appended or copied
 *   over the old destination contents"
 *
 * The source is always interpreted using the destination element width, so the
 * same code works for byte strings and for wider code units
 *
 * Public wrappers such as @ref mem_concat_unbounded_string,
 * @ref mem_concat_bounded_string, @ref mem_copy_unbounded_string,
 * @ref mem_copy_bounded_string, @ref mem_concat_fixed_string, and
 * @ref mem_copy_fixed_string select the matching binary mode flags
 *
 * @par Source flags
 * - `SOURCE_BOUNDED_STRING`: use @p source_range_bytes as the byte limit and
 *   stop at the first zero-valued terminator element if one is found earlier
 * - `SOURCE_UNBOUNDED_STRING`: ignore @p source_range_bytes and scan until the
 *   first zero-valued terminator element
 * - `SOURCE_FIXED_STRING`: trust that the provided range already ends with one
 *   terminator element and do not scan for it again
 *
 * @par Transfer flags
 * - `TRANSFER_APPEND`: keep the old destination text and add the visible
 *   source payload after it
 * - `TRANSFER_REPLACE`: discard the old visible destination text and make
 *   the visible source payload the new destination text
 *
 * @par Empty-source rules
 * In append mode, an empty source means "change nothing". In replace mode, an
 * empty source means "reset to an empty string", so the result is a descriptor
 * whose visible length is zero and whose storage still contains one terminator
 *
 * @par Self-aliasing
 * The source may point inside the current destination allocation. Since the
 * destination is always required to be in string mode, self-aliased sources
 * are always validated against the visible string bounds. This is an
 * intentional supported scenario, not an accident. For example, replacing
 * `"abcdef"` with a suffix that starts at `"cdef"` is valid
 *
 * The tricky case is internal replace with shrink. A normal `m_resize(...)`
 * can move or rewrite the terminator before the aliased source bytes are read.
 * To avoid corrupting that source slice, the helper first moves the visible
 * payload to the front of the current buffer, restores the terminator there,
 * and only then finalizes the logical shrink
 *
 * @par Validation rules
 * - @p destination must be initialized and must already be a string descriptor
 *   in both append and replace modes. Use @ref mem_convert_data_to_string first
 *   when starting from a data-mode buffer
 * - bounded and fixed-string flags require @p source_range_bytes to describe
 *   whole logical elements
 * - unbounded mode requires an external non-NULL source to be truly
 *   terminated by a zero-valued element of the destination element width
 * - internal aliased sources must start on an element boundary and must stay
 *   inside the logical destination contents accepted by the selected mode
 * - @p mode combines source-interpretation flags with append-or-replace flags
 *
 * @param mode Binary mode flags that select both source interpretation and
 *        append-or-replace behavior
 * @param destination Pointer to the destination descriptor
 * @param source_range_bytes Source byte range. In bounded mode this is the
 *        scan limit. In fixed-string mode this is the full source size. In
 *        unbounded mode it is ignored. When used, it must be divisible by the
 *        destination element size
 * @param source_string Pointer to source string elements. In append mode,
 *        `NULL` means "append nothing". In replace mode, `NULL` means
 *        "replace with an empty string"
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_core_string(
	const MEM_CORE_MODE mode,
	memory *destination,
	const size_t source_range_bytes,
	const void *const source_string)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Source-mode subset extracted from the combined mode flags */
	const MEM_CORE_MODE source_mode = mode & SOURCE_MASK;

	/* Transfer-mode subset extracted from the combined mode flags */
	const MEM_CORE_MODE transfer_mode = mode & TRANSFER_MASK;

	/* Visible source prefix length measured in elements */
	size_t source_length = 0;

	/* Offset of a self-aliased source from the beginning of destination data.
	   Saved before any resize so the source can be rebuilt afterwards */
	size_t source_offset = 0;

	/* Visible string elements remaining from an internal source start up to the terminator */
	size_t remaining_visible_elements = 0;

	/* True when source_string points into the current destination allocation */
	bool source_is_inside_destination = false;

	/* True when the internal source is validated against string-mode visible bounds */
	bool source_is_inside_string_destination = false;

	/* True when the transfer should behave as if the source payload were empty */
	bool source_is_effectively_empty = false;

	if(destination == NULL)
	{
		report("Memory management; Destination must be non-NULL");
		provide(FAILURE);
	}

	if(destination->length > 0 && destination->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(destination->single_element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(destination->is_string == false)
	{
		report("Memory management; Destination must be a string descriptor");
		provide(FAILURE);
	}

	if(source_string == NULL)
	{
		if(transfer_mode & TRANSFER_APPEND)
		{
			provide(status);
		}

		source_is_effectively_empty = true;
	}

	/* Outside unbounded mode, the byte count must describe a whole number of logical
	   string elements. Partial elements would make later scan and copy steps ambiguous.
	   Unbounded mode intentionally ignores source_range_bytes, so the check is skipped */
	if(source_is_effectively_empty == false &&
		(source_mode & SOURCE_UNBOUNDED_STRING) == 0 &&
		(source_range_bytes % destination->single_element_size) != 0)
	{
		report("Memory management; Size %zu is not divisible by element size %zu",
			source_range_bytes,
			destination->single_element_size);
		provide(FAILURE);
	}

	if((source_mode & SOURCE_UNBOUNDED_STRING) == 0 && source_range_bytes == 0)
	{
		if(transfer_mode & TRANSFER_APPEND)
		{
			provide(status);
		}

		source_is_effectively_empty = true;
	}

	/* Check whether the source aliases the current destination allocation */
	if((TRIUMPH & status) &&
		source_string != NULL &&
		source_is_effectively_empty == false)
	{
		/* Current logical destination size in bytes, used to validate self-aliased sources */
		size_t destination_bytes = 0;

		run(mem_guarded_byte_size(destination,destination->length,&destination_bytes));

		if((TRIUMPH & status) && destination->data != NULL && destination->actually_allocated_bytes > 0)
		{
			/* Start address of the current destination allocation used for range checks */
			const uintptr_t destination_begin = (uintptr_t)destination->data;

			/* Start address of the source string used for range checks */
			const uintptr_t source_begin = (uintptr_t)source_string;

			if(destination->actually_allocated_bytes > UINTPTR_MAX - destination_begin ||
				destination_bytes > UINTPTR_MAX - destination_begin)
			{
				report("Memory management; Destination address range overflows");
				provide(FAILURE);
			}

			/* End address of the current allocation, one past the last allocated byte */
			const uintptr_t allocation_end = destination_begin + destination->actually_allocated_bytes;

			/* End address of the current logical destination contents */
			const uintptr_t logical_end = destination_begin + destination_bytes;

			if(source_begin >= destination_begin && source_begin < allocation_end)
			{
				source_is_inside_destination = true;
				source_offset = source_begin - destination_begin;

				if((source_offset % destination->single_element_size) != 0)
				{
					report("Memory management; Source start is not aligned to element size %zu",
						destination->single_element_size);
					provide(FAILURE);
				}

				source_is_inside_string_destination = true;

				{
					/* Element offset of an internal source from the beginning of destination data */
					const size_t source_offset_elements = source_offset / destination->single_element_size;

					if(source_mode & SOURCE_UNBOUNDED_STRING)
					{
						if(source_begin >= logical_end)
						{
							report("Memory management; Unbounded source start exceeds destination logical bounds");
							provide(FAILURE);
						}

						/* Reject starts past the visible terminator on purpose. Treating them
						   as empty input would silently hide caller-side pointer bugs */
						if(source_offset_elements > destination->string_length)
						{
							report("Memory management; Source start exceeds destination visible string bounds");
							provide(FAILURE);
						}

						remaining_visible_elements = destination->string_length - source_offset_elements;
					} else if(source_mode & SOURCE_BOUNDED_STRING) {
						if(source_begin > logical_end)
						{
							report("Memory management; Bounded source start exceeds destination logical bounds");
							provide(FAILURE);
						}

						/* Reject starts past the visible terminator on purpose. Treating them
						   as empty input would silently hide caller-side pointer bugs */
						if(source_offset_elements > destination->string_length)
						{
							report("Memory management; Source start exceeds destination visible string bounds");
							provide(FAILURE);
						}

						remaining_visible_elements = destination->string_length - source_offset_elements;
					} else {
						if(source_begin > logical_end || source_range_bytes > destination_bytes - source_offset)
						{
							report("Memory management; Source range exceeds destination logical bounds");
							provide(FAILURE);
						}

						/* Reject starts past the visible terminator on purpose. Treating them
						   as empty input would silently hide caller-side pointer bugs */
						if(source_offset_elements > destination->string_length)
						{
							report("Memory management; Source start exceeds destination visible string bounds");
							provide(FAILURE);
						}

						remaining_visible_elements = destination->string_length - source_offset_elements;
					}
				}
			}
		}
	}

	if((TRIUMPH & status) &&
		source_string != NULL &&
		source_is_effectively_empty == false)
	{
		/* Read-only byte view of the source string used by the scanners */
		const unsigned char *const source_data_view = (const unsigned char *)source_string;

		if(source_mode & SOURCE_BOUNDED_STRING)
		{
			if(source_is_inside_string_destination == true)
			{
				/* Number of logical source elements implied by the provided byte count */
				const size_t source_elements = source_range_bytes / destination->single_element_size;

				/* Clamp the requested internal slice to the remaining visible suffix instead of rescanning */
				source_length = source_elements;

				if(remaining_visible_elements < source_length)
				{
					source_length = remaining_visible_elements;
				}
			} else {
				/* Search up to the first zero-valued element within the provided byte limit */
				run(mem_string_measure_length(
					source_data_view,
					source_range_bytes,
					destination->single_element_size,
					true,
					&source_length,
					NULL));
			}
		} else if(source_mode & SOURCE_UNBOUNDED_STRING) {
			if(source_is_inside_string_destination == true)
			{
				/* Reuse the known internal string length instead of rescanning destination-owned text */
				source_length = remaining_visible_elements;
			} else {
				/* Search up to the first zero-valued element without an external bound */
				run(mem_string_measure_length(
					source_data_view,
					0,
					destination->single_element_size,
					false,
					&source_length,
					NULL));
			}
		} else {
			/* Fixed-string mode trusts that the provided range already ends with one
			   terminator element, so the last logical element is accepted as-is */
			/* Number of logical source elements implied by the provided byte count */
			const size_t source_elements = source_range_bytes / destination->single_element_size;
			source_length = source_elements - 1;
		}
	}

	if((TRIUMPH & status) &&
		(transfer_mode & TRANSFER_APPEND) != 0 &&
		source_length == 0)
	{
		provide(status);
	}

	/* Visible destination length before appending. Used as the append offset in elements */
	size_t destination_length = 0;

	if(transfer_mode & TRANSFER_APPEND)
	{
		run(mem_string_length(destination,&destination_length));
	}

	/* Combined visible string length after the transfer, excluding the trailing terminator */
	size_t visible_string_length = source_length;

	if(TRIUMPH & status && (transfer_mode & TRANSFER_APPEND) != 0)
	{
			run(mem_guarded_add(destination_length,source_length,&visible_string_length));

		if(CRITICAL & status)
		{
			report("Memory management; Concatenation would overflow element count");
		}
	}

	/* Total destination element count after the transfer, including the trailing terminator */
	size_t new_total_elements = 0;

	if(TRIUMPH & status)
	{
		run(mem_guarded_add(visible_string_length,1,&new_total_elements));

		if(CRITICAL & status)
		{
			report("Memory management; Not enough room for string terminator");
		}
	}

	/* Byte offset where the copied source prefix starts in destination */
	size_t offset_bytes = 0;

	/* Number of source bytes to copy after the source length is known */
	size_t source_bytes = 0;

	if(transfer_mode & TRANSFER_APPEND)
	{
		run(mem_guarded_byte_size(destination,destination_length,&offset_bytes));
	}

	run(mem_guarded_byte_size(destination,source_length,&source_bytes));

	if((TRIUMPH & status) &&
		(transfer_mode & TRANSFER_REPLACE) != 0 &&
		source_is_inside_string_destination == true)
	{
		/* Writable byte view of the existing destination buffer before the shrink finalizes */
		unsigned char *destination_data_rewritable = (unsigned char *)destination->data;

		if(destination_data_rewritable == NULL)
		{
			report("Memory management; Destination data pointer is NULL during internal replace");
			status = FAILURE;
		} else {
			if(source_bytes > 0)
			{
				memmove(destination_data_rewritable,
					destination_data_rewritable + source_offset,
					source_bytes);
			}

			if(TRIUMPH & status)
			{
				run(mem_write_zero_terminator(destination,source_length));

				if(TRIUMPH & status)
				{
					destination->string_length = source_length;
					run(m_resize(destination,new_total_elements));
				}
			}
		}
	} else {
		/* m_resize may call realloc, so any source_string pointer into destination data
		   becomes invalid here. Self-aliasing remains safe only because the code
		   saved source_offset earlier and can rebuild the pointer afterwards */
		run(m_resize(destination,new_total_elements));

		if(TRIUMPH & status)
		{
			/* Writable byte view of the destination buffer after the possible realloc */
			unsigned char *destination_data_rewritable = (unsigned char *)destination->data;

			if(destination_data_rewritable == NULL)
			{
				report("Memory management; Destination data pointer is NULL after m_resize");
				status = FAILURE;
			} else {
				if(source_bytes > 0)
				{
					/* Resolved byte view of the source payload after the optional self-alias rebase */
					const unsigned char *source_data_view = (const unsigned char *)source_string;

					if(source_is_inside_destination == true)
					{
						/* Rebase the aliased source onto the possibly relocated destination
						   buffer before memmove reads from it */
						source_data_view = destination_data_rewritable + source_offset;
					}

					memmove(destination_data_rewritable + offset_bytes,source_data_view,source_bytes);
				}

				if(TRIUMPH & status)
				{
					run(mem_write_zero_terminator(destination,visible_string_length));

					if(TRIUMPH & status)
					{
						destination->string_length = visible_string_length;
					}
				}
			}
		}
	}

	provide(status);
}
