#include "mem.h"
#include "mem_internal.h"
#include <string.h>

/**
 * @brief Append one string descriptor to another using cached string lengths
 *
 * This helper is the strict descriptor-to-descriptor string append counterpart
 * of @ref mem_copy. Both operands must already be string descriptors that use
 * the same non-zero element width. The source descriptor is treated as a
 * fixed-string payload whose logical extent ends exactly at
 * `source->string_length + 1`, so any reserved tail beyond the cached
 * terminator is ignored instead of being rescanned or appended
 *
 * @par No terminator search needed
 * Because both descriptors carry an authoritative cached `string_length`,
 * this function performs **no terminator search at all** — it simply uses
 * the cached visible length of the source. This is the fastest available
 * path for appending one libmem string to another and should be preferred
 * whenever both operands are already managed string descriptors.
 *
 * For appending raw C-string buffers, use @ref mem_concat_fixed_string,
 * @ref mem_concat_bounded_string, or @ref mem_concat_unbounded_string
 * depending on whether and how the source is terminated
 *
 * @param destination Pointer to the string descriptor that receives the append
 * @param source Pointer to the string descriptor whose visible payload is appended
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_concat_strings(
	memory       *destination,
	const memory *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Logical source element count used for fixed-string routing */
	size_t source_total_elements = 0;

	/* Total source byte count passed to the routed append helper */
	size_t source_size_bytes = 0;

	if(destination == NULL || source == NULL)
	{
		report("Memory management; mem_concat_strings arguments must be non-NULL");
		provide(FAILURE);
	}

	if(source->length > 0 && source->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(destination->is_string == false || source->is_string == false)
	{
		report("Memory management; Source and destination must both be strings");
		provide(FAILURE);
	}

	if(destination->single_element_size != source->single_element_size)
	{
		report("Memory management; Element size mismatch (%zu vs %zu)",
			destination->single_element_size,
			source->single_element_size);
		provide(FAILURE);
	}

	if((destination->length == 0 && destination->string_length != 0) ||
	        (destination->length > 0 && destination->string_length >= destination->length))
	{
		report("Memory management; Destination string descriptor is inconsistent");
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
	   Route the append through fixed-string semantics so only the visible prefix plus
	   one terminator participates in the concatenation, while any reserved tail
	   beyond that cached terminator remains ignored */
	run(mem_guarded_add(source->string_length,1,&source_total_elements));
	run(mem_guarded_byte_size(source,source_total_elements,&source_size_bytes));
	run(mem_core_string(
		SOURCE_FIXED_STRING | TRANSFER_APPEND,
		destination,
		source_size_bytes,
		source->data));

	provide(status);
}
