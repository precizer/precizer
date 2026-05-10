#include "test_libmem_utils.h"

/**
 * @brief Copy an unsigned long long int descriptor whose array has size 0
 *
 * Creates a source and a destination descriptor, resizes the source to size 0,
 * copies it into the destination, and frees both
 *
 * @return Return describing success or failure
 */
Return test_libmem_0001(void)
{
	INITTEST;

	// Create the source and destination descriptors
	m_create(unsigned long long int,test0_0);
	m_create(unsigned long long int,test0_1);

	// Resize the source to size 0 and copy it into the destination
	ASSERT(SUCCESS == m_resize(test0_0,0));
	ASSERT(SUCCESS == m_copy(test0_1,test0_0));

	// Cleanup
	ASSERT(SUCCESS == m_del(test0_0));
	ASSERT(SUCCESS == m_del(test0_1));

	RETURN_STATUS;
}
