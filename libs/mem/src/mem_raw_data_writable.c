#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Return a writable raw pointer without checking the element type
 *
 * This helper returns a writable pointer without performing any element-type
 * verification. It is meant for low-level code that already knows what lives in
 * the descriptor and only needs the library to reject the most obviously broken
 * descriptor states before exposing the buffer.
 *
 * In practice, that means the function still checks for a `NULL` descriptor, a
 * zero element size, reserved bytes with no backing pointer, and non-zero
 * logical length with a `NULL` data pointer. It does not compare the descriptor
 * against an expected element type, and it does not rewrite `is_string` or
 * `string_length`
 *
 * On success the returned pointer is the descriptor's live @ref memory::data
 * pointer, not a copy and not a detached view. A read-only raw view obtained
 * from the same descriptor exposes the same backing storage. The pointer stays
 * valid only until an operation that may replace or release the descriptor
 * storage, such as @ref mem_resize, @ref mem_delete, or a descriptor-copy or
 * concat operation that targets this descriptor
 *
 * @param memory_object Pointer to a descriptor
 * @return Writable storage pointer on success. `NULL` when the descriptor is too broken to expose safely
 */
inline __attribute__((always_inline)) void *mem_raw_data_writable(memory *memory_object)
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
