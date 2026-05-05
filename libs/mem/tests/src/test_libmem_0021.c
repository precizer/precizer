#include "test_libmem_utils.h"

/**
 * @brief Check data-to-string conversion accepts an already-string non-byte descriptor
 *
 * @return Return describing success or failure
 */
Return test_libmem_0021(void)
{
	INITTEST;

	memory already_string_descriptor = m_init(int,MEMORY_STRING);

	ASSERT(SUCCESS == m_to_string(&already_string_descriptor));
	ASSERT(already_string_descriptor.single_element_size == sizeof(int));
	ASSERT(already_string_descriptor.length == 0);
	ASSERT(already_string_descriptor.string_length == 0);
	ASSERT(already_string_descriptor.is_string == true);
	ASSERT(already_string_descriptor.data == NULL);

	RETURN_STATUS;
}
