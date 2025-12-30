#include "sute.h"

int main(void)
{
	SUTESTART;

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
	SUTE(test0007,"libmem Memory allocator test set…");
	TEST(test0008,"librational test report messaging…");
	TEST(test0009,"librational test slog messaging…");
	TEST(test0017,"librational test itoa function…");

	HEADER("Unit Testing of precizer's functions");
	SUTE(test0012,"add_string_to_array() test set…");
#if 0
	TEST(test0021,"Native international UTF8 encoding test set…");
#endif
	TEST(test0022,"remove_trailing_slash() test set…");
	TEST(test0023,"extract_relative_path() test set…");
	SUTE(test0025,"file_buffer_memory() test set…");
	TEST(test0026,"file_check_access() test set…");

	HEADER("Unit Testing of precizer");
	TEST(comprehensive_unit_testing,"Comprehensive Unit testing…");

	HEADER("System Testing of precizer");
	TEST(comprehensive_system_testing,"Comprehensive System testing…");

	HEADER("Clean results");
	RUN(clean,"Temporary data cleanup…");

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	SUTEDONE;
}
