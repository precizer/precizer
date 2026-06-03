#include "precizer.h"

/**
 * @brief Remove extra trailing slashes from a path descriptor
 *
 * Measures the visible string through the libmem string-length helper, then
 * trims trailing `/` characters from that visible prefix. Keeps the root path
 * `/` unchanged and leaves empty paths unchanged
 *
 * @param[in,out] path Path descriptor to normalize
 * @return SUCCESS when the path is normalized or does not need changes.
 *         Returns FAILURE when the descriptor is invalid or when a visible
 *         non-empty payload cannot be accessed as a writable char buffer
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
