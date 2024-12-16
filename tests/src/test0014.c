#include "sute.h"

Return short_relative_path_test(void){
	Return status = SUCCESS;
	RETURN_STATUS;
}

Return short_absolute_path_test(void){
	Return status = SUCCESS;
	RETURN_STATUS;
}

Return long_relative_path_test(void){
	Return status = SUCCESS;
	RETURN_STATUS;
}

Return long_absolute_path_test(void){
	Return status = SUCCESS;
	RETURN_STATUS;
}

// Main test runner
Return test0014(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(short_relative_path_test,"Short relative path");
	TEST(short_absolute_path_test,"Short absolute path");
	TEST(long_relative_path_test,"Long relative path");
	TEST(long_absolute_path_test,"Long absolute path");

	RETURN_STATUS;
}
