#include "mem.h"

/**
 * @brief Delete every inline descriptor in a descriptor-backed array and then delete the root descriptor
 *
 * The root descriptor must stay in data mode and must store elements of type
 * `memory`. This helper walks every inline descriptor, deletes each child
 * descriptor through @ref m_del, and finally deletes the root descriptor itself
 * so the whole array can be torn down through one call
 *
 * Small example:
 * @code
 * m_create(memory,string_array);
 *
 * if((TRIUMPH & m_string_array_append(string_array,char,"delta")) == 0) { return FAILURE; }
 * if((TRIUMPH & m_string_array_append(string_array,char,sizeof("epsilon"),"epsilon")) == 0) { return FAILURE; }
 * if((TRIUMPH & m_array_del(string_array)) == 0) { return FAILURE; }
 * @endcode
 *
 * @param descriptor_array Root data-mode descriptor that stores `memory` elements
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_array_delete(memory *descriptor_array)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	memory *inline_descriptors_data_rewritable = NULL;
	size_t descriptor_count = 0;

	if(descriptor_array == NULL)
	{
		report("Memory management; Array delete descriptor is NULL");
		provide(FAILURE);
	}

	if(descriptor_array->length > 0 && descriptor_array->data == NULL)
	{
		report("Memory management; Array delete descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(descriptor_array->single_element_size != sizeof(memory))
	{
		report("Memory management; Array delete requires a root descriptor with memory-sized elements");
		provide(FAILURE);
	}

	if(descriptor_array->is_string == true)
	{
		report("Memory management; Array delete requires a data-mode root descriptor");
		provide(FAILURE);
	}

	if(descriptor_array->string_length != 0)
	{
		report("Memory management; Data-mode root descriptor has stale string metadata during array delete");
		provide(FAILURE);
	}

	descriptor_count = descriptor_array->length;

	if(descriptor_count > 0)
	{
		inline_descriptors_data_rewritable = m_data(memory,descriptor_array);

		if(inline_descriptors_data_rewritable == NULL)
		{
			report("Memory management; Array delete could not map the root descriptor data");
			status = FAILURE;
		}
	}

	if((TRIUMPH & status) && inline_descriptors_data_rewritable != NULL)
	{
		for(size_t descriptor_index = 0; descriptor_index < descriptor_count; ++descriptor_index)
		{
			call(m_del(&inline_descriptors_data_rewritable[descriptor_index]));
		}
	}

	call(m_del(descriptor_array));

	provide(status);
}
