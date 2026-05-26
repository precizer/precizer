#include "mem.h"
#include "mem_internal.h"
#include <string.h>

/**
 * @brief Copy one descriptor into another when both descriptors use the same mode
 *
 * This helper copies one descriptor into another when both descriptors already
 * use the same semantic mode.
 * When both operands are treated as strings, the copy trusts the cached
 * `source->string_length`, treats the source as a fixed-string payload that
 * ends exactly at that cached visible length plus one terminator element, and
 * delegates the replacement to @ref mem_copy_fixed_string. When both
 * operands are already in data mode, the raw descriptor transfer is delegated
 * to @ref mem_copy_data
 *
 * Mixed string/data copies are rejected on purpose. This helper does not perform
 * implicit mode conversion because that would hide a change in the semantic kind
 * of the payload behind a plain copy call. In string mode the source and
 * destination must also use the same element size, and a string source must
 * provide consistent cached string metadata
 *
 * @param destination Pointer to the destination descriptor that receives the copy
 * @param source Pointer to the source descriptor to copy from
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_copy(
	memory       *destination,
	const memory *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Total source byte count passed to the routed copy helper */
	size_t source_size_bytes = 0;

	/* Logical source element count used for fixed-string routing */
	size_t source_total_elements = 0;

	if(destination == NULL || source == NULL)
	{
		report("Memory management; copy arguments must be non-NULL");
		provide(FAILURE);
	}

	if(destination->length > 0 && destination->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(source->length > 0 && source->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(destination->single_element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source->single_element_size == 0)
	{
		report("Memory management; Source element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(destination->is_string != source->is_string)
	{
		report("Memory management; Source and destination must both be strings or both be data");
		provide(FAILURE);
	}

	if(destination->is_string == true)
	{
		if(destination->single_element_size != source->single_element_size)
		{
			report("Memory management; Element size mismatch (%zu vs %zu)",
				destination->single_element_size,
				source->single_element_size);
			provide(FAILURE);
		}

		if((source->length == 0 && source->string_length != 0) ||
		        (source->length > 0 && source->string_length >= source->length))
		{
			report("Memory management; Source string descriptor is inconsistent");
			provide(FAILURE);
		}

		/* In descriptor-to-descriptor string mode, the source contract already
		   guarantees a cached visible length and a terminator right after it.
		   Route the copy through fixed-string semantics so the transfer uses exactly
		   that visible prefix plus one terminator element, instead of rescanning
		   the whole logical span in search of zero */
		run(mem_guarded_add(source->string_length,1,&source_total_elements));
		run(mem_guarded_byte_size(source,source_total_elements,&source_size_bytes));
		run(mem_copy_fixed_string(destination,source_size_bytes,source->data));
	} else {
		run(mem_copy_data(destination,source));
	}

	provide(status);
}
