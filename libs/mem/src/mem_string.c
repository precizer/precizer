#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Zero-filled fallback storage for soft read-only string access
 *
 * `mem_string(...)` must be able to return an empty string even for string
 * descriptors whose elements are wider than one byte. A plain
 * `static char empty_string[] = ""` is only guaranteed to describe a byte
 * string, so it is not a good universal fallback for `wchar_t`, `char16_t`,
 * `char32_t`, or other descriptor element widths
 *
 * The `empty` field provides a block of zero-filled storage whose size matches
 * `sizeof(max_align_t)`. The `align` field is not used as data. It exists so
 * the whole object inherits the strongest standard alignment that the platform
 * normally provides for fundamental scalar types. That way callers can safely
 * interpret the returned address as a pointer to a zero-valued string element
 * of the descriptor's width instead of only as a `char *`
 *
 * Using a file-scope union also solves the lifetime problem. A stack buffer
 * would become invalid as soon as `mem_string(...)` returned, while this object
 * lives for the whole program and needs no heap allocation or cleanup logic.
 * The exact byte size stays platform-dependent because `sizeof(max_align_t)` is
 * ABI-dependent, but that is still preferable here to introducing a separate
 * hard-coded magic limit
 */
static const union
{
	max_align_t align;
	unsigned char empty[sizeof(max_align_t)];
} empty_string = {0};

/**
 * @brief Return a soft read-only string view without modifying descriptor state
 *
 * This helper is meant for callers that already trust string mode and only need
 * a readable pointer. It does not rescan the buffer for a terminator, does not
 * recalculate the visible length, and never rewrites descriptor state. The
 * function performs only the most basic descriptor checks and otherwise fully
 * trusts the cached `string_length`
 *
 * When the descriptor is logically empty, the function returns a zero-filled
 * fallback string instead of NULL. Gross descriptor contract violations such
 * as a NULL descriptor, a zero element size, calling the helper on a data
 * descriptor, a non-zero logical length with a NULL data pointer, or a cached
 * string length that is not strictly smaller than `length` are reported and
 * also fall back to the same empty string storage
 *
 * @param memory_object Descriptor interpreted as a read-only string descriptor
 * @return Read-only pointer to descriptor-backed string data or to shared empty
 *         fallback storage when no valid string view can be exposed
 */
const void *mem_string(const memory *memory_object)
{
	if(memory_object == NULL)
	{
		report("Memory management; Soft string view requires a non-NULL descriptor");
		return(empty_string.empty);
	}

	if(memory_object->single_element_size == 0)
	{
		report("Memory management; Soft string view requires a non-zero element size");
		return(empty_string.empty);
	}

	if(memory_object->is_string == false)
	{
		report("Memory management; Soft string view requires a string descriptor");
		return(empty_string.empty);
	}

	if(memory_object->length > 0 && memory_object->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		return(empty_string.empty);
	}

	if(memory_object->data == NULL || memory_object->length == 0)
	{
		return(empty_string.empty);
	}

	if(memory_object->string_length >= memory_object->length)
	{
		report("Memory management; Soft string view requires string_length to stay below length");
		return(empty_string.empty);
	}

	return(memory_object->data);
}
