#include "mem.h"

void *memory_data_checked(
	memory *memory_structure,
	size_t expected_element_size)
{
	if(memory_structure == NULL)
	{
		slog(ERROR,"Memory management; Descriptor is NULL");
		return NULL;
	}

	if(memory_structure->element_size != expected_element_size)
	{
		slog(ERROR,
			"Memory management; Expected %zu bytes but descriptor uses %zu",
			expected_element_size,
			memory_structure->element_size);
		return NULL;
	}

	return memory_structure->data;
}
