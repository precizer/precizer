#include "sute.h"

/**
 * Global definitions for Unit Tests
 *
 */

// Global variable controls signals to interrupt execution
// Atomic variable is very fast and will be called very often
_Atomic bool global_interrupt_flag = false;

// The global structure Config where all runtime settings will be stored
Config _config;
Config *config = &_config;

int main(void)
{
	// Test function name and its short description
	TESTSTART;

	HEADER("Preparations\n");
	TEST(prepare,"Preparation for tests");

	HEADER("\nTest examples\n");
	TEST(test0004, "Example test…");
	TEST(test0005, "Example test…");
	TEST(test0006, "Example test…");
	TEST(test0010, "Example test…");

	HEADER("\nTesting of Librarys\n");
	TEST(test0001, "libsha512 hash check with sha512…");
	TEST(test0007, "libmem Memory allocator test set");
	TEST(test0008, "librational test report messaging…");
	TEST(test0009, "librational test slog messaging…");

	HEADER("\nUnit Testing of precizer\n");
	TEST(test0012, "add_string_to_array() test set…");

	HEADER("\nSystem Testing of precizer\n");
	TEST(test0002, "Create default name database…");
	TEST(test0003, "Comply default DB name to \"hostname.db\" template…");
	TEST(test0011, "User's Manual and examples from README test set…");

	HEADER("\nClean results\n");
	TEST(clean, "Cleaning all results…");

	TESTFINISH(finish,"Telemetry");

	HEADER("\nFinishing\n");
	TESTDONE;
}
