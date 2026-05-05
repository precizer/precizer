#include "sute.h"

/**
 * @brief Run typed-memory example scenarios for libmem
 *
 * @return Return describing success or failure
 */
Return test_libmem_typed_data(void)
{
	INITTEST;
	bool first_header = true;

	HEADER("Typed Access");
	TEST(test_libmem_0010,"Typed point descriptors and m_resize flags…");
	TEST(test_libmem_0011,"Typed point raw access and descriptor copying…");
	TEST(test_libmem_0012,"Typed point raw concat with shrink and regrow…");

	HEADER("Initial Modes");
	TEST(test_libmem_0013,"create supports optional data or string initial mode…");

	HEADER("Descriptor Collections");
	TEST(test_libmem_0069,"Descriptor-backed arrays support item access and foreach traversal…");

	RETURN_STATUS;
}
