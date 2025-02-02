#include "precizer.h"

/**
 * @brief Removes all leading dots ('.') from the beginning of the string.
 * @param str Pointer to the character array.
 * @param size Size of the array, including the null terminator.
 */
static void remove_leading_dots(
	char   *str,
	size_t size
){
	if(str == NULL || size < 2)
	{
		return;
	}

	// Find first non-dot character
	size_t i = 0;

	while(str[i] == '.' && i < size - 1)
	{
		i++;
	}

	// Shift the string to the left if there were leading dots
	if(i > 0)
	{
		size_t j;

		for(j = 0; j < size - i; j++)
		{
			str[j] = str[j + i];
		}
		// Ensure null terminator at the end
		str[j] = '\0';
	}
}

/**
 * @brief Removes trailing '.' character from a given string.
 *
 * This function iterates through the string from the end and replaces any trailing
 * '.' character with a null terminator '\0'.
 *
 * @param str Pointer to the string to be modified.
 *
 * @note If the string is NULL or empty, the function does nothing.
 */
static void trim_trailing_chars(
	char   *str,
	size_t size
){
	if(str == NULL || *str == '\0' || size < 2)
	{
		return; // If string is NULL or empty, do nothing
	}

	size_t len = size;

	while(len > 0 && str[len - 1] == '.')
	{
		str[len - 1] = '\0';
		len--;
	}
}

/**
 * @brief Shortens a path string if it exceeds maximum length
 *
 * If the path length exceeds maxLen characters, the function truncates
 * the middle portion of the string and inserts an ellipsis character.
 * The resulting string has format: "<first_half>…<second_half>"
 *
 * @param path[in,out] Path string to be shortened
 *
 * @return Return status code:
 *         - SUCCESS: Path was shortened successfully or didn't need shortening
 *         - FAILURE: Memory operation failed
 */
Return shorten_path(char *path){

	Return status = SUCCESS;

	size_t len = strlen(path);
	const char *ellipsis = "…"; /* 3 bytes  of Unicode ellipsis '\u2026' */
	char *result = NULL;
	char *end = NULL;

	size_t maxLen = terminal_width();

	/* Validate input parameters */
	if(NULL == path)
	{
		return(status);
	}

	/* Path is within limits, no action needed */
	if(len <= maxLen)
	{
		return(status);
	}

	/* Integer overflow possible if this value is less than,
	   no action needed */
	if(maxLen < 8)
	{
		return(status);
	}

	/* Calculate lengths for first and second parts */
	size_t ellipsis_length = strlen(ellipsis);
	size_t startLen = (maxLen / 2) - ellipsis_length;
	size_t endLen = maxLen - startLen - ellipsis_length;

	char *start = (char *)malloc(startLen);

	if(NULL == start)
	{
		report("Memory allocation failed, requested size: %zu bytes",startLen);
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		end = (char *)malloc(endLen);

		if(NULL == end)
		{
			report("Memory allocation failed, requested size: %zu bytes",endLen);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Copy path parts */
		strncpy(start,path,startLen - 1);
		start[startLen - 1] = '\0';
		trim_trailing_chars(start,startLen);

		strncpy(end,path + len + 1 - endLen,endLen - 1);
		end[endLen - 1] = '\0';
		remove_leading_dots(end,endLen);

		/* Format shortened path */
		if(-1 == asprintf(&result,"%s%s%s",start,ellipsis,end))
		{
			slog(ERROR,"asprintf() failed\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status && NULL != result)
	{
		memcpy(path,result,maxLen - 1);
		path[maxLen - 1] = '\0';
	}

	/* Cleanup */
	free(start);
	free(end);
	free(result);

	return(status);
}
