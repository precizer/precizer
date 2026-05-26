#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Shared internal append core for arrays of inline string descriptors
 *
 * This helper keeps all descriptor-array management in one place. Public
 * wrappers only decide whether the source should be copied through the bounded
 * or unbounded string backend. The core then validates the root descriptor,
 * grows it by one `memory` element, initializes the new slot as a string
 * descriptor, routes the actual copy to the matching backend, and rolls the
 * array back if any later step fails
 *
 * The root descriptor must stay in data mode and must store elements of type
 * `memory`. The appended child descriptor always starts as an empty string
 * descriptor created by @ref m_init
 *
 * @param source_mode Either `SOURCE_UNBOUNDED_STRING` or `SOURCE_BOUNDED_STRING`
 * @param descriptor_array Root data-mode descriptor that stores `memory` elements
 * @param single_element_size Element width in bytes for the appended child string descriptor
 * @param source_limit_bytes Maximum byte count used only for bounded source mode
 * @param source_string Source string passed to the selected copy backend
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_string_array_core(
	MEM_CORE_MODE     source_mode,
	memory            *descriptor_array,
	size_t            single_element_size,
	size_t            source_limit_bytes,
	const void *const source_string)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	memory *inline_descriptors_data_rewritable = NULL;
	size_t next_index = 0;

	if(descriptor_array == NULL)
	{
		report("Memory management; String-array append descriptor is NULL");
		provide(FAILURE);
	}

	if(descriptor_array->length > 0 && descriptor_array->data == NULL)
	{
		report("Memory management; String-array append descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(descriptor_array->single_element_size != sizeof(memory))
	{
		report("Memory management; String-array append requires a root descriptor with memory-sized elements");
		provide(FAILURE);
	}

	if(descriptor_array->is_string == true)
	{
		report("Memory management; String-array append requires a data-mode root descriptor");
		provide(FAILURE);
	}

	if(descriptor_array->string_length != 0)
	{
		report("Memory management; Data-mode root descriptor has stale string metadata during string-array append");
		provide(FAILURE);
	}

	if(single_element_size == 0)
	{
		report("Memory management; String-array append requires a non-zero string element size");
		provide(FAILURE);
	}

	if(source_mode != SOURCE_UNBOUNDED_STRING &&
	        source_mode != SOURCE_BOUNDED_STRING)
	{
		report("Memory management; String-array append received unsupported source mode");
		provide(FAILURE);
	}

	/* Save the old logical length as the insertion index. After growth, the new
	   inline string descriptor belongs exactly at this saved slot */
	next_index = descriptor_array->length;

	run(m_resize(descriptor_array,next_index + 1,ZERO_NEW_MEMORY));

	if((TRIUMPH & status) && descriptor_array->length > next_index)
	{
		inline_descriptors_data_rewritable = m_data(memory,descriptor_array);

		if(inline_descriptors_data_rewritable == NULL)
		{
			report("Memory management; String-array append could not map the grown root descriptor");
			status = FAILURE;
		}
	}

	if((TRIUMPH & status) && inline_descriptors_data_rewritable != NULL)
	{
		/* Use the low-level constructor here because the element width is a runtime value.
		   m_init(T,...) requires a compile-time type, so it cannot express descriptors whose string element size is provided by the caller */
		inline_descriptors_data_rewritable[next_index] = mem_init(single_element_size,MEMORY_STRING);

		if(source_mode == SOURCE_UNBOUNDED_STRING)
		{
			run(mem_copy_unbounded_string(&inline_descriptors_data_rewritable[next_index],source_string));
		}

		if(source_mode == SOURCE_BOUNDED_STRING)
		{
			run(mem_copy_bounded_string(&inline_descriptors_data_rewritable[next_index],source_limit_bytes,source_string));
		}
	}

	if((TRIUMPH & status) == 0 && descriptor_array->length > next_index)
	{
		inline_descriptors_data_rewritable = m_data(memory,descriptor_array);

		if(inline_descriptors_data_rewritable != NULL)
		{
			call(m_del(&inline_descriptors_data_rewritable[next_index]));
		}

		call(m_resize(descriptor_array,next_index));
	}

	provide(status);
}
