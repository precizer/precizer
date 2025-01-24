#include "precizer.h"

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
Return shorten_path(char* path){
	Return status = SUCCESS;
	size_t len = strlen(path);
	size_t maxLen = 50;
	const char* ellipsis = "…"; /* 3 bytes  of Unicode ellipsis '\u2026' */
	char* result;
	size_t ellipsis_length = 0;

	/* Validate input parameters */
	if(NULL == path || maxLen < 8)
	{
		return(FAILURE);
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
	ellipsis_length = strlen(ellipsis);
	size_t startLen = (maxLen / 2) - ellipsis_length;
	size_t endLen = maxLen - startLen - ellipsis_length;

	char* start = (char*)malloc(startLen);
	if(NULL == start)
	{
		report("Memory allocation failed, requested size: %zu bytes",startLen);
		return(FAILURE);
	}

	char* end = (char*)malloc(endLen);
	if(NULL == end)
	{
		report("Memory allocation failed, requested size: %zu bytes",endLen);
		return(FAILURE);
	}

	/* Copy path parts */
	strncpy(start,path,startLen - 1);
	start[startLen - 1] = '\0';

	strncpy(end,path + len + 1 - endLen,endLen - 1);
	end[endLen - 1] = '\0';

	/* Format shortened path */
	if(-1 == asprintf(&result,"%s%s%s",start,ellipsis,end))
	{
		status = FAILURE;
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
