#include "test_libmem_utils.h"

/**
 * @brief Check that copying an empty data descriptor clears a populated destination
 *
 * Creates empty source and populated destination descriptors with unsigned long
 * long int elements. Copying the empty source into the destination must clear
 * its logical payload while preserving its data-mode metadata
 *
 * @return Return describing success or failure
 */
Return test_libmem_0001(void)
{
	INITTEST;

	// Create the source and destination descriptors
	m_create(unsigned long long int,test0_0);
	m_create(unsigned long long int,test0_1);

	// Keep the source empty and prepare a non-empty destination
	ASSERT(SUCCESS == m_resize(test0_0,0));
	ASSERT(test0_0->length == 0U);
	ASSERT(SUCCESS == m_resize(test0_1,2));
	ASSERT(test0_1->length == 2U);

	// Copying the empty source clears the destination payload
	ASSERT(SUCCESS == m_copy(test0_1,test0_0));
	ASSERT(test0_1->length == 0U);
	ASSERT(test0_1->string_length == 0U);
	ASSERT(test0_1->is_string == false);

	// Cleanup
	call(m_del(test0_0));
	call(m_del(test0_1));

	RETURN_STATUS;
}
