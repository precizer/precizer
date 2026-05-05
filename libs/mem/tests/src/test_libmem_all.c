#include "sute.h"

/**
 * @brief Run all public libmem test suites
 *
 * @return Return status code
 */
Return test_libmem_all(void)
{
	INITTEST;

	SUTE(test_libmem_foundation,"libmem foundation scenarios…");
	SUTE(test_libmem_typed_data,"libmem typed data scenarios…");
	SUTE(test_libmem_data_mode,"libmem data-mode scenarios…");
	SUTE(test_libmem_string_mode,"libmem string-mode scenarios…");
	SUTE(test_libmem_mode_boundaries,"libmem mode-boundary scenarios…");

	RETURN_STATUS;
}
