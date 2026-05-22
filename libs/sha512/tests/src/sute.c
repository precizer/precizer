#include "sute.h"

/**
 * @brief Run the standalone libsha512 test program
 * @details Starts the testitall runner for the SHA-512 library checks. This is
 * the entry point used when a developer runs the libsha512 tests directly
 *
 * @return Process exit status produced by the test runner
 */
int main(void)
{
	SUTESTART;

	/* Give standalone runs a clear top-level title before the grouped tests */
	HEADER("Testing of libsha512");
	TEST(test_libsha512_all,"libsha512 verifies known digests, chunked input, boundaries and safe failures...");

	RUN(finish,"Telemetry");

	/* Keep the final section explicit so the runner output matches the style of
	   the other bundled library test programs */
	HEADER("Finishing");
	SUTEDONE;
}
