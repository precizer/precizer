#include "sute.h"

int main(void){
	// Test function name and its short description
	TESTSTART;

	HEADER("Preparations\n");
	TEST(prepare,"Preparation for tests");

	HEADER("\nTest examples\n");
	TEST(test0004,"Example test…");
	TEST(test0005,"Example test…");
	TEST(test0006,"Example test…");
	TEST(test0010,"Example test…");

	HEADER("\nTesting of built-in libraries\n");
	TEST(test0001,"libsha512 hash check with sha512…");
	TEST(test0007,"libmem Memory allocator test set");
	TEST(test0008,"librational test report messaging…");
	TEST(test0009,"librational test slog messaging…");
//	TEST(test0017,"librational test itoa function…");
//	TEST(test0015,"libxdiff compare texts…");

	HEADER("\nUnit Testing of precizer\n");
	TEST(test0012,"add_string_to_array() test set…");

	HEADER("\nSystem Testing of precizer\n");
	TEST(test0002,"Create default name database…");
	TEST(test0003,"Comply default DB name to \"hostname.db\" template…");
	TEST(test0011,"User's Manual and examples from README test set…");
	TEST(test0013,"Dry Run mode testing…");
	TEST(test0014,"Short, long, relative and absolute paths…");
	TEST(test0016,"--watch-timestamps argument testing…");

	HEADER("\nClean results\n");
	TESTNR(clean,"Temporary data cleanup…");

	TESTNR(finish,"Telemetry");

	HEADER("\nFinishing\n");
	TESTDONE;
}
