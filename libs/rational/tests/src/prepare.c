#include "sute.h"

/**
 * @brief Prepare an isolated workspace for standalone librational tests
 *
 * The test executable runs from the library's tests directory. ORIGIN_DIR is
 * set to its parent, and all templates are copied into the temporary workspace
 *
 * @param[out] temporary_directory Initialized string descriptor that retains
 *             the created directory path for cleanup, including after failure
 * @return SUCCESS when the templates are ready, otherwise FAILURE
 */
Return prepare(memory *temporary_directory)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,origin_directory,MEMORY_STRING);

	run(determine_origin_dir(origin_directory));
	run(set_environment_variable("ORIGIN_DIR",m_text(origin_directory)));
	run(create_tmpdir(temporary_directory));
	run(set_environment_variable("TMPDIR",m_text(temporary_directory)));
	run(create_directory("tests/templates"));
	run(copy_from_origin("tests/templates","tests/templates/librational",REQUIRE_SOURCE_EXISTS));

	call(m_del(origin_directory));
	provide(status);
}
