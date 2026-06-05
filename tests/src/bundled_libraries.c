#include "sute.h"
#include "test_libmem_all.h"
#include "test_librational_all.h"
#include "test_libsha512_all.h"
#include "test_libtestitall_all.h"

/**
 * @brief Run the test groups that belong to bundled libraries
 * @details The main application test runner includes these checks so library
 * behavior is verified together with the application that embeds it
 *
 * @return Return status for all bundled library test groups
 */
Return bundled_libraries(void)
{
	INITTEST;

	SUTE(test_librational_all,"Testing of librational");
	SUTE(test_libsha512_all,"libsha512 public hashing behavior checks");
	SUTE(test_libmem_all,"Testing of libmem");
	SUTE(test_libtestitall_all,"Testing of libtestitall");

	RETURN_STATUS;
}
