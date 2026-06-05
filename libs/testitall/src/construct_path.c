#include "testitall.h"

/**
 * @brief Construct a path by joining `$TMPDIR` and a child path
 *
 * Reads the temporary-directory root from the process environment and writes
 * the combined path through the public libmem formatted-string API. The output
 * descriptor must be initialized as a `char` `MEMORY_STRING` descriptor
 *
 * @param[in] filename Child path relative to `$TMPDIR`
 * @param[out] full_path Byte string descriptor that receives the combined path
 * @return SUCCESS when the combined path is stored. FAILURE when an input,
 *         environment value, or memory operation is invalid
 */
Return construct_path(
	const char *filename,
	memory     *full_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *tmp_dir = NULL;

	if(filename == NULL)
	{
		echo(STDERR,"construct_path: filename must not be NULL\n");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		tmp_dir = getenv("TMPDIR");

		if(NULL == tmp_dir)
		{
			echo(STDERR,"construct_path: TMPDIR is not set for \"%s\"\n",filename);
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		/* m_formatted_string() supplies the path separator and trailing string terminator */
		run(m_formatted_string(full_path,"%s/%s",tmp_dir,filename));
	}

	deliver(status);
}
