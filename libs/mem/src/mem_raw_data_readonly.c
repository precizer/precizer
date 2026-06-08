#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Return a read-only raw pointer without checking the element type
 *
 * This helper is the read-only counterpart of @ref mem_raw_data_writable.
 * It returns the descriptor's storage pointer as `const void *` and skips any
 * element-type validation, so callers can use it for low-level inspection code
 * that already trusts the stored representation.
 *
 * The function still protects callers from the roughest descriptor failures. It
 * refuses to expose a buffer when the descriptor itself is `NULL`, when the
 * element size was never initialized, when reserve metadata exists without a
 * backing pointer, or when the logical length says the descriptor contains data
 * but the data pointer is `NULL`
 *
 * On success the returned pointer is the descriptor's live @ref memory::data
 * pointer, not a copy and not a detached view. A writable raw view obtained
 * from the same descriptor exposes the same backing storage. The pointer stays
 * valid only until an operation that may replace or release the descriptor
 * storage, such as @ref mem_resize, @ref mem_delete, or a descriptor-copy or
 * concat operation that targets this descriptor
 *
 * @param memory_object Pointer to a descriptor
 * @return Read-only storage pointer on success. `NULL` when the descriptor is too broken to expose safely
 */
const void *mem_raw_data_readonly(const memory *memory_object)
{
	if(memory_object == NULL)
	{
		report("Memory management; Descriptor is NULL");
		return(NULL);
	}

	if(memory_object->single_element_size == 0)
	{
		report("Memory management; Descriptor element size is zero (uninitialized)");
		return(NULL);
	}

	if(memory_object->data == NULL && memory_object->actually_allocated_bytes > 0)
	{
		report("Memory management; Descriptor has reserved bytes with NULL data pointer");
		return(NULL);
	}

	if(memory_object->length > 0 && memory_object->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		return(NULL);
	}

	return(memory_object->data);
}
