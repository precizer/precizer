#include "sute.h"

/**
 * @brief Run direct unit tests for individual precizer functions
 *
 * @details
 * This suite keeps function-level checks in one place. These tests call the
 * target functions directly through the test runner and do not participate in
 * the shared internal/external application scenario matrix
 *
 * @return Return status for all direct function-level tests
 */
Return function_unit_testing(void)
{
	INITTEST;

	SUTE(test0004,"create_directory(): test set");
	SUTE(test0005,"file_buffer_memory(): test set");
	SUTE(test0006,"match_regexp(): test set");
	SUTE(test0012,"add_string_to_array(): test set");
#if 0
	TEST(test0021,"Native international UTF8 encoding test set");
#endif
	TEST(test0022,"remove_trailing_slash(): test set");
	TEST(test0023,"path_build_relative(): stable paths across root spellings");
	SUTE(test0026,"file_check_access(): directory-relative access statuses");
	TEST(test0036,"delete_path(): removes a regular file");
	SUTE(test0037,"delete_path(): diagnostics test set");

	RETURN_STATUS;
}
