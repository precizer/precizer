#include "sute.h"

/**
 * @brief Run every SHA-512 library test group
 * @details Keeps the standalone library test output grouped by the kind of
 * promise being checked: known answers first, then incremental input handling
 *
 * @return Return status for the whole libsha512 test group
 */
Return test_libsha512_all(void)
{
	INITTEST;

	bool first_header = true;

	/* Start with public known-answer tests because they describe the basic
	   promise users expect from any SHA-512 implementation */
	HEADER("Reference Digests");
	TEST(test_libsha512_0001,"Known SHA-512 examples produce the published digests...");

	/* Then check streaming behavior: callers may hash files in chunks and still
	   need the exact same digest as a one-shot hash */
	HEADER("Incremental Hashing");
	TEST(test_libsha512_0002,"Chunked input produces the same digest as one-shot input...");
	TEST(test_libsha512_0004,"Boundary-sized messages hash consistently in one-shot and chunked modes...");

	/* Finish with invalid-input behavior so API failures stay predictable and
	   readable */
	HEADER("Failure Handling");
	TEST(test_libsha512_0003,"Invalid SHA-512 API use returns named errors without corrupting state...");

	RETURN_STATUS;
}
