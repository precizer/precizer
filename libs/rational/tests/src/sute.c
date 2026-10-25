#include "sute.h"

/**
 * @brief Run the top-level test suite executable for librational
 *
 * @return Process exit status produced by the test runner macros
 */
int main(void)
{
	SUTESTART;
	m_create(char,temporary_directory,MEMORY_STRING);

	HEADER("Preparations");
	run(prepare(temporary_directory));

	HEADER("Testing of librational");
	TEST(test_librational_all,"librational test set");

	HEADER("Clean results");
	call(clean(temporary_directory));

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	SUTEDONE;
}
