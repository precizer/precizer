#include "sute.h"

/**
 * @brief Check that delete_path() removes a regular file
 *
 * @return Return describing success or failure
 */
Return test0036(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	INITTEST;

	bool file_exists = false;
	m_create(char,absolute_path,MEMORY_STRING);

	ASSERT(SUCCESS == replase_to_string("delete_path regular file coverage","delete_path_regular_file.txt"));
	ASSERT(SUCCESS == construct_path("delete_path_regular_file.txt",absolute_path));
	ASSERT(SUCCESS == check_file_exists(&file_exists,m_text(absolute_path)));
	ASSERT(file_exists == true);
	ASSERT(SUCCESS == delete_path("delete_path_regular_file.txt"));
	/* Verify that delete_path() really removed the file */
	ASSERT(SUCCESS == check_file_exists(&file_exists,m_text(absolute_path)));
	ASSERT(file_exists == false);

	call(m_del(absolute_path));

	RETURN_STATUS;
}
