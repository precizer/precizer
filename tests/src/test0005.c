#include "sute.h"

/* A fixed file-read buffer limit keeps the traversal's memory budget
   independent of system-memory availability */

/**
 * @brief Verify that file reading uses a 64 KiB buffer limit
 * @return SUCCESS when the requested size is exactly 65536 bytes
 */
static Return test0005_1(void)
{
	INITTEST;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)(64U*1024U));

	RETURN_STATUS;
}

/**
 * @brief Run the fixed file-read buffer size check
 *
 * @details
 * The check verifies the byte count requested by the traversal for its
 * reusable heap buffer
 *
 * @return Accumulated test status
 */
Return test0005(void)
{
	INITTEST;

	TEST(test0005_1,"file_buffer_memory(): returns a fixed 64 KiB buffer size");

	/* The size check does not allocate a file-read buffer.
	   Allocation and reuse are exercised by the application-level tests */

	RETURN_STATUS;
}
