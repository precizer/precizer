#include "mem.h"
#include "mem_internal.h"
#include <string.h>

/**
 * @brief Convert a data descriptor into string mode and cache the visible length
 *
 * Use this helper when a descriptor currently holds raw data, but from now on
 * should behave like a string. The function scans the current logical range
 * until it finds the first zero-valued element and treats that position as the
 * end of the visible string. A zero-valued element means that every byte in
 * that element is zero
 *
 * If the descriptor is not empty and there is not enough logical room for one
 * trailing terminator, the helper grows it and writes that terminator for you.
 * Empty descriptors are promoted to string mode without allocating storage.
 * Descriptors that are already in string mode are accepted as a no-op only
 * when their cached string metadata is internally consistent
 *
 * The helper rejects inconsistent descriptors instead of guessing what the user
 * meant. Examples include `length > 0` with `data == NULL`, `single_element_size == 0`,
 * or stale `string_length` while the descriptor is still in data mode
 *
 * Small example:
 * @code
 * m_create(char,buffer);
 * if((TRIUMPH & mem_copy_buffer(buffer,sizeof("abc"),"abc")) == 0) { return FAILURE; }
 * if((TRIUMPH & mem_convert_data_to_string(buffer)) == 0) { return FAILURE; }
 * // buffer->length == 4, buffer->string_length == 3, buffer->is_string == true
 * @endcode
 *
 * @param memory_structure Mutable descriptor that should start behaving like a string
 * @return `SUCCESS` on success; `FAILURE` if the descriptor is inconsistent or cannot be resized
 */
Return mem_convert_data_to_string(memory *memory_structure)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Visible prefix length used to update string_length and place the terminator */
	size_t measured_length = 0;

	if(memory_structure == NULL)
	{
		report("Memory management; Invalid arguments for string conversion");
		provide(FAILURE);
	}

	if(memory_structure->length > 0 && memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(memory_structure->is_string == false &&
	        memory_structure->string_length != 0)
	{
		report("Memory management; Data descriptor has non-zero string_length during string conversion");
		provide(FAILURE);
	}

	if(memory_structure->single_element_size == 0)
	{
		report("Memory management; Descriptor element size is zero during string conversion");
		provide(FAILURE);
	}

	if(memory_structure->is_string == true &&
	        memory_structure->length == 0 &&
	        memory_structure->string_length != 0)
	{
		report("Memory management; String descriptor has non-zero string_length with zero length during string conversion");
		provide(FAILURE);
	}

	if(memory_structure->is_string == true &&
	        memory_structure->length > 0 &&
	        memory_structure->string_length >= memory_structure->length)
	{
		report("Memory management; String descriptor cache is inconsistent during string conversion");
		provide(FAILURE);
	}

	if(memory_structure->is_string == true)
	{
		provide(status);
	}

	if(memory_structure->length == 0)
	{
		memory_structure->string_length = 0;
		memory_structure->is_string = true;
	}

	if(memory_structure->length > 0)
	{
		/* Reserve room for exactly one trailing string terminator.
		   This block first measures how much payload is currently visible before the first zero-valued element.
		   It then computes payload plus one terminator slot and grows the descriptor only when the current logical length is too small */
		size_t required_elements = 0;

		run(mem_find_zero_terminator(memory_structure,&measured_length));
		run(mem_guarded_add(measured_length,1,&required_elements));

		if(CRITICAL & status)
		{
			report("Memory management; Not enough room for string terminator");
		}

		/* Grow only when the current logical length cannot hold both the visible payload and one final zero-valued terminator.
		   If enough logical space already exists, conversion keeps the current descriptor size and reuses it as is */
		if((TRIUMPH & status) && memory_structure->length < required_elements)
		{
			run(m_resize(memory_structure,required_elements));
		}
	}

	if((TRIUMPH & status) && memory_structure->length > 0)
	{
		/* Materialize a well-formed string view in the current buffer.
		   This block writes the final zero-valued terminator at the measured boundary,
		   then stores the visible payload length in string_length and marks the descriptor as a string */
		unsigned char *memory_structure_data_rewritable = (unsigned char *)memory_structure->data;

		if(memory_structure_data_rewritable == NULL)
		{
			report("Memory management; Data pointer is NULL while converting data to string");
			status = FAILURE;
		} else {
			/* Finalize the descriptor as a usable string.
			   This writes the terminating zero element at the measured boundary,
			   caches how many payload elements stay visible before that terminator,
			   and switches the descriptor into string mode for future string-aware helpers */
			run(mem_write_zero_terminator(memory_structure,measured_length));
			memory_structure->string_length = measured_length;
			memory_structure->is_string = true;
		}
	}

	if(TRIUMPH & status)
	{
		telemetry_data_to_string_conversions();
	}

	provide(status);
}
