#include "precizer.h"

/**
 * @brief Normalize a path descriptor by removing redundant trailing slashes
 *
 * Keeps `/` unchanged, leaves empty paths unchanged, and trims repeated trailing
 * separators from ordinary paths such as `dir///`
 *
 * @param[in,out] path Path descriptor to normalize
 * @return SUCCESS when the path is normalized or already valid, otherwise FAILURE
 */
Return remove_trailing_slash(memory *path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Visible path length before trailing slashes are trimmed */
	size_t visible_path_length = 0;

	/* Writable C string pointer to the descriptor payload */
	char *path_data_rewritable = NULL;

	run(m_string_length(path,&visible_path_length));

	if((TRIUMPH & status) && visible_path_length > 0)
	{
		path_data_rewritable = m_data(char,path);

		if(path_data_rewritable == NULL)
		{
			slog(ERROR,"Path normalization; Failed to access non-empty descriptor as a char buffer\n");
			status = FAILURE;
		}
	}

	/* Keep the single root slash intact */
	if((TRIUMPH & status) && visible_path_length > 0)
	{
		while(visible_path_length > 1 && path_data_rewritable[visible_path_length - 1] == '/')
		{
			--visible_path_length;
		}

		path_data_rewritable[visible_path_length] = '\0';

		call(m_resize(path,visible_path_length + 1,RELEASE_UNUSED));
	}

	provide(status);
}
