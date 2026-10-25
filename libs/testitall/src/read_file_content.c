#include "testitall.h"

/**
 * @brief Read a file relative to the tests directory under TMPDIR
 *
 * For example, "templates/example.txt" reads
 * "$TMPDIR/tests/templates/example.txt". TMPDIR must be set and non-empty.
 * File reading follows get_file_content(), including rejection of empty files.
 * A failure clears the destination buffer when it is non-NULL
 *
 * @param[in] relative_path File path relative to $TMPDIR/tests
 * @param[out] content Destination initialized as a char MEMORY_STRING descriptor
 * @return SUCCESS when the file is read, FAILURE on invalid input, a missing
 *         temporary-directory setting, or a path construction or read error
 */
Return read_file_content(
	const char *relative_path,
	memory     *content)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,relative_test_path,MEMORY_STRING);
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path == NULL || content == NULL)
	{
		echo(STDERR,"read_file_content: path and content must not be NULL\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const char *temporary_directory = getenv("TMPDIR");

		if(temporary_directory == NULL || temporary_directory[0] == '\0')
		{
			echo(STDERR,"read_file_content: TMPDIR must be set and non-empty\n");
			status = FAILURE;
		}
	}

	run(m_formatted_string(relative_test_path,"tests/%s",relative_path));
	run(construct_path(m_text(relative_test_path),absolute_path));
	run(get_file_content(m_text(absolute_path),content));

	if((FAILURE & status) && content != NULL)
	{
		call(m_del(content));
	}
	call(m_del(absolute_path));
	call(m_del(relative_test_path));
	provide(status);
}
