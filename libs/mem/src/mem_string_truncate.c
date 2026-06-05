#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Shorten the visible string without shrinking the reserved buffer
 *
 * Use this helper when the string should become logically shorter, but the
 * current allocation should stay in place. The helper writes a zero-valued
 * terminator at @p new_visible_length and updates `string_length`, while the
 * descriptor's total `length` is left unchanged. In string mode that is
 * intentional: `string_length` tracks only the visible prefix, while `length`
 * tracks the whole logical descriptor span. For example, a descriptor may
 * hold visible text `"alphabet"` with `string_length == 8` and `length == 32`
 * after earlier reserve growth; truncating it to `5` makes the visible text
 * `"alpha"` and updates `string_length` to `5`, but `length` still remains `32`
 *
 * If @p new_visible_length is greater than the current visible string length,
 * the call succeeds as a no-op. If the requested visible length already
 * matches the current one, the helper may still rewrite the terminator slot so
 * the string stays well-formed. Descriptors whose reserved byte count no
 * longer covers the current logical payload are rejected instead of being
 * treated as no-ops
 *
 * Small example:
 * @code
 * if((TRIUMPH & mem_string_truncate(name,5)) == 0) { return FAILURE; }
 * // "alphabet" becomes visible as "alpha", but the descriptor keeps the same reserve
 * @endcode
 *
 * @param destination String descriptor to truncate
 * @param new_visible_length Requested visible string length measured in elements
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_string_truncate(
	memory *destination,
	size_t new_visible_length)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Byte size of the payload currently claimed by destination, used for reserve validation */
	size_t current_payload_bytes = 0;

	if(destination == NULL)
	{
		report("Memory management; string truncate destination must be non-NULL");
		provide(FAILURE);
	}

	if((TRIUMPH & status) && destination->single_element_size == 0)
	{
		report("Memory management; String truncate destination element size is zero");
		provide(FAILURE);
	}

	if((TRIUMPH & status) && destination->length > 0 && destination->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if((TRIUMPH & status) &&
	        destination->actually_allocated_bytes > 0 &&
	        destination->data == NULL)
	{
		report("Memory management; Descriptor has reserved bytes with NULL data pointer during string truncate");
		provide(FAILURE);
	}

	if((TRIUMPH & status) && destination->is_string == false)
	{
		report("Memory management; string truncate requires a string descriptor");
		provide(FAILURE);
	}

	if((TRIUMPH & status) && destination->length > 0)
	{
		run(mem_guarded_byte_size(destination,destination->length,&current_payload_bytes));
	}

	if((TRIUMPH & status) &&
	        destination->length > 0 &&
	        destination->actually_allocated_bytes < current_payload_bytes)
	{
		report("Memory management; Descriptor reserve is smaller than logical payload during string truncate");
		provide(FAILURE);
	}

	if(TRIUMPH & status)
	{
		if(destination->length == 0)
		{
			provide(status);
		} else {
			/* Cached visible string length before truncate decides whether it is a no-op or a real cut */
			const size_t current_visible_length = destination->string_length;

			if(current_visible_length >= destination->length)
			{
				report("Memory management; String descriptor is inconsistent during truncate");
				provide(FAILURE);
			}

			if((TRIUMPH & status) && new_visible_length > current_visible_length)
			{
				provide(status);
			}

			if((TRIUMPH & status) && new_visible_length == current_visible_length)
			{
				/* This branch is intentionally not a plain "redundant truncate" no-op.
				   mem_resize() relies on it when string capacity changes but the visible
				   payload length does not. In that flow the resize logic wants to keep
				   string_length unchanged while still guaranteeing that the element at
				   string_length is a valid zero terminator after any reallocation or
				   shrink-to-fit operation.

				   The fast path below preserves that contract efficiently: if the
				   current terminator is already present, truncate returns without an
				   unnecessary memset(). If low-level code damaged that one element but
				   left the rest of the metadata consistent, the function falls through
				   to mem_write_zero_terminator() and repairs the terminator in place.

				   Because this branch dereferences the terminator slot directly, all
				   reserve-consistency checks above must stay ahead of it. Removing this
				   branch would silently change the agreed behavior of mem_resize(), and
				   moving it ahead of the reserve validation would reintroduce an
				   out-of-bounds read risk on corrupted descriptors */
				/* Byte offset of the current terminator element inside destination->data */
				size_t terminator_offset = 0;

				run(mem_guarded_byte_size(destination,current_visible_length,&terminator_offset));

				if(TRIUMPH & status)
				{
					/* Read-only byte view used to inspect whether the existing terminator is already valid */
					const unsigned char *destination_data_view = (const unsigned char *)destination->data;

					if(mem_is_zero_element(destination_data_view + terminator_offset,destination->single_element_size) == true)
					{
						provide(status);
					}
				}
			}
		}
	}

	run(mem_write_zero_terminator(destination,new_visible_length));

	if(TRIUMPH & status)
	{
		destination->string_length = new_visible_length;
	}

	provide(status);
}
