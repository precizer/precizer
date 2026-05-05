#include "test_libmem_utils.h"

/**
 * @brief Tests copying of empty memory unsigned long long int structures
 * @details Verifies that copying an empty memory structure works correctly
 *         by creating two unsigned long long int memory structures and copying one to another
 *
 * @return Return enum indicating success or failure of the test
 * @retval SUCCESS if test passed
 * @retval FAILURE if test failed
 */
Return test_libmem_0001(void)
{
	INITTEST;

	// Allocate memory for the structure int
	m_create(unsigned long long int,test0_0);
	m_create(unsigned long long int,test0_1);

	// Create and copy of an unsigned long long int memory arrays
	ASSERT(SUCCESS == m_resize(test0_0,0));
	ASSERT(SUCCESS == m_copy(test0_1,test0_0));

	// Cleanup
	ASSERT(SUCCESS == m_del(test0_0));
	ASSERT(SUCCESS == m_del(test0_1));

	RETURN_STATUS;
}
