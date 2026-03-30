#include "precizer.h"

/**
 * @brief Normalize a path descriptor by trimming trailing '/' characters
 *
 * Scans the descriptor as a bounded byte string up to the first `'\0'` byte or
 * `path->length`, whichever comes first. Compacts the descriptor to the visible
 * string plus one trailing `'\0'`, removes trailing forward slashes from that
 * visible prefix, then compacts it again. For libmem-managed buffers under
 * plain `SUCCESS`, the helper also releases unused capacity. Leaves a slash-only
 * prefix as a single `/`. Empty descriptors are ignored
 *
 * @param[in,out] path Path descriptor whose visible string payload is normalized
 * @return SUCCESS after normalization or when no change is needed.
 *         Returns FAILURE when the descriptor is NULL or when a non-empty descriptor
 *         cannot be accessed as a character buffer.
 *         Otherwise returns the status propagated from @ref string_length and
 *         resize operations while continuing through graceful statuses
 */
Return remove_trailing_slash(memory *path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	size_t len = 0;

	run(string_length(path,&len));

	// Exit on FAILURE
	if((TRIUMPH & status) == 0)
	{
		provide(status);
	}

	if(path->length == 0)
	{
		provide(status);
	}

	char *path_array = data(char,path);

	if(path_array == NULL)
	{
		slog(ERROR,"Path normalization; Failed to access non-empty descriptor as a char buffer\n");
		provide(FAILURE);
	}

	// Keep the single root slash intact
	while(len > 1 && path_array[len - 1] == '/')
	{
		--len;
	}

	path_array[len] = '\0';

	call(resize(path,len + 1,RELEASE_UNUSED));

	provide(status);
}
