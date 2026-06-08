#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Return a read-only typed pointer after verifying the element size
 *
 * This helper performs the same descriptor validation as
 * @ref mem_data_writable, but returns a read-only pointer. Use it when the
 * caller wants checked typed access and does not intend to modify the
 * descriptor's storage through the returned pointer.
 *
 * The function never rewrites the descriptor. It either returns the existing
 * storage pointer as `const void *`, or reports why the descriptor is not safe
 * to read and returns `NULL`
 *
 * @param memory_structure Pointer to a descriptor
 * @param expected_single_element_size Expected element size in bytes, usually `sizeof(T)`
 * @return Read-only data pointer on success. `NULL` when the descriptor is invalid or the element size does not match
 */
const void *mem_data_readonly(
	const memory *memory_structure,
	size_t       expected_single_element_size)
{
	size_t current_payload_bytes = 0;

	if(memory_structure == NULL)
	{
		report("Memory management; Descriptor is NULL");
		return(NULL);
	}

	if(memory_structure->single_element_size == 0)
	{
		report("Memory management; Descriptor element size is zero (uninitialized)");
		return(NULL);
	}

	if(memory_structure->data == NULL && memory_structure->actually_allocated_bytes > 0)
	{
		report("Memory management; Descriptor has reserved bytes with NULL data pointer");
		return(NULL);
	}

	if(memory_structure->length > 0 && memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		return(NULL);
	}

	Return guarded_status = mem_guarded_byte_size(
		memory_structure,
		memory_structure->length,
		&current_payload_bytes);

	if((TRIUMPH & guarded_status) == 0)
	{
		return(NULL);
	}

	if(memory_structure->length > 0 &&
	        memory_structure->actually_allocated_bytes < current_payload_bytes)
	{
		report("Memory management; Descriptor reserve is smaller than logical payload");
		return(NULL);
	}

	if(memory_structure->single_element_size != expected_single_element_size)
	{
		report("Memory management; Expected %zu bytes but descriptor uses %zu",expected_single_element_size,memory_structure->single_element_size);
		return(NULL);
	}

	return(memory_structure->data);
}
