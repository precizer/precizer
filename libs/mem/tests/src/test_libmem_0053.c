#include "test_libmem_utils.h"

/**
 * @brief Check raw pointer reset helper on manually allocated memory
 *
 * @return Return describing success or failure
 */
Return test_libmem_0053(void)
{
	INITTEST;

	char *manual_buffer = (char *)malloc(64u);
	ASSERT(manual_buffer != NULL);

	if(manual_buffer != NULL)
	{
		strcpy(manual_buffer,"temporary heap payload");
	}

	m_reset(&manual_buffer);
	ASSERT(manual_buffer == NULL);

	RETURN_STATUS;
}
