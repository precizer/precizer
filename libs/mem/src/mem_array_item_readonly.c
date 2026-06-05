#include "mem.h"

/**
 * @brief Return a read-only pointer to one typed descriptor element
 *
 * The function validates that @p index is inside the descriptor's logical
 * element range and then delegates descriptor integrity and element-size checks
 * to @ref mem_data_readonly. It returns `NULL` for invalid descriptors,
 * mismatched element sizes, empty descriptors, and out-of-range indexes
 *
 * @param memory_structure Pointer to the descriptor that stores an array
 * @param index Zero-based index of the requested element
 * @param expected_single_element_size Expected element size in bytes, usually `sizeof(T)`
 * @return Read-only element pointer on success. `NULL` when the element cannot be addressed safely
 */
const void *mem_array_item_readonly(
	const memory *memory_structure,
	size_t       index,
	size_t       expected_single_element_size)
{
	if(memory_structure == NULL)
	{
		report("Memory management; Descriptor is NULL");
		return(NULL);
	}

	if(index >= memory_structure->length)
	{
		report("Memory management; Item index %zu is out of range for descriptor length %zu",index,memory_structure->length);
		return(NULL);
	}

	const void *data = mem_data_readonly(memory_structure,expected_single_element_size);

	if(data == NULL)
	{
		return(NULL);
	}

	const unsigned char *item_data = data;

	return(&item_data[index * expected_single_element_size]);
}
