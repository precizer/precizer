#include "sute.h"

int main(void){
	TESTSTART;

	HEADER("Preparations");
	TEST(prepare,"Preparation for tests");

	HEADER("Test examples");
	// Test function name and its short description
	TEST(test0004,"Example test…");
	TEST(test0005,"Example test…");
	TEST(test0006,"Example test…");
	TEST(test0010,"Testitall library capability demonstration…");

	HEADER("Testing of built-in libraries");
	TEST(test0001,"libsha512 hash check with sha512…");
	TEST(test0002,"An empty example…");
	TEST(test0007,"libmem Memory allocator test set…");
	TEST(test0008,"librational test report messaging…");
	TEST(test0009,"librational test slog messaging…");
	TEST(test0017,"librational test itoa function…");

	HEADER("Unit Testing of precizer");
	TEST(test0012,"add_string_to_array() test set…");

	HEADER("System Testing of precizer");
	TEST(test0003,"Comply default DB name to \"hostname.db\" template…");
	TEST(test0011,"User's Manual and examples from README test set…");
	TEST(test0013,"Dry Run mode testing…");
	TEST(test0014,"Short, long, relative and absolute paths…");
	TEST(test0015,"Database upgrade testing…");
	TEST(test0016,"--watch-timestamps argument testing…");
	TEST(test0018,"--maxdepth argument testing…");
	TEST(test0019,"Testing symlink operations…");
	TEST(test0020,"Testing DB creation attempt in missing directory…");

	HEADER("Clean results");
	RUN(clean,"Temporary data cleanup…");

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	TESTDONE;
}
