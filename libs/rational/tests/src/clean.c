#include "sute.h"

/**
 * @brief Remove the standalone test workspace and release its path
 *
 * An empty descriptor means preparation did not create a directory. Cleanup
 * targets the saved workspace even if a test changed TMPDIR
 *
 * @param[in,out] temporary_directory Directory path retained by prepare()
 * @return SUCCESS when cleanup completes, otherwise FAILURE
 */
Return clean(memory *temporary_directory)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(temporary_directory->length > 0U)
	{
		run(set_environment_variable("TMPDIR",m_text(temporary_directory)));
		run(delete_path(""));
	}

	call(m_del(temporary_directory));
	provide(status);
}
