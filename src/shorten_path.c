#include "precizer.h"

/**
 * @brief Gets the terminal width (number of columns)
 * @return Number of columns in the terminal or default value 80 columns
 */
static size_t terminal_width(void){

	struct winsize w;

	if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&w) == -1)
	{
		/* Unable to retrieve terminal window width.
		   Defaulting to 80 columns. */
		return(80UL);

	} else {
		return((size_t)w.ws_col);
	}
}

/**
 * @brief Check if a given UTF-8 character is a period ('.')
 *
 * @param str Pointer to the start of a UTF-8 character
 * @return true if it is a period, false otherwise
 */
STATIC bool is_utf8_dot(const char *str){
	mbstate_t state;
	memset(&state,0,sizeof(state));
	wchar_t wc;
	size_t len = mbrtowc(&wc,str,MB_CUR_MAX,&state);
	return (len > 0 && wc == L'.');
}

/**
 * @brief Remove all leading dots ('.') from a UTF-8 string
 *
 * @param str UTF-8 encoded input string
 */
STATIC void remove_leading_dots(char *str){
	if(!str || *str == '\0')
	{
		return;
	}

	char *start = str;

	while(*start && is_utf8_dot(start))
	{
		size_t len = mbrlen(start,MB_CUR_MAX,NULL);

		if(len == 0 || len == (size_t)-1)
		{
			break;
		}
		start += len;
	}

	// Move trimmed string to the beginning
	if(start != str)
	{
		memmove(str,start,strlen(start) + 1);
	}
}

/**
 * @brief Remove all trailing dots ('.') from a UTF-8 string
 *
 * @param str UTF-8 encoded input string
 */
STATIC void remove_trailing_dots(char *str){
	if(!str || *str == '\0')
	{
		return;
	}

	char *end = str + strlen(str);

	// Найти последний символ, который не является точкой
	while(end > str)
	{
		char *p = end - 1;

		while(p > str && (*p & 0xC0) == 0x80)    // Пропуск многобайтовых символов
		{
			p--;
		}

		if(!is_utf8_dot(p))
		{
			break; // Найден символ, который не точка
		}

		// Правильное удаление последней точки
		*p = '\0';
		end = p;
	}
}

/**
 * @brief Truncates a UTF-8 string to the specified number of characters in place.
 *
 * @param str Input UTF-8 string (modified in place).
 * @param max_chars Maximum number of characters to keep.
 */
static void utf8_truncate(
	char   *str,
	size_t max_chars
){
	if(!str || max_chars == 0)
	{
		return;
	}

	unsigned char *ptr = (unsigned char *)str;
	unsigned char *end = ptr;
	size_t count = 0;

	// Iterate through the string character by character
	while(*ptr)
	{
		unsigned char *next = ptr;

		if((*ptr & 0x80) == 0)    // 1-byte character (ASCII)
		{
			next = ptr + 1;
		} else if((*ptr & 0xE0) == 0xC0){   // 2-byte character
			next = ptr + 2;
		} else if((*ptr & 0xF0) == 0xE0){   // 3-byte character
			next = ptr + 3;
		} else if((*ptr & 0xF8) == 0xF0){   // 4-byte character
			next = ptr + 4;
		} else {
			break; // Invalid UTF-8 sequence
		}

		if(next > (unsigned char *)str + strlen(str))
		{
			break; // Prevent buffer overflow
		}

		if(++count > max_chars)
		{
			break;
		}

		end = next;
		ptr = next;
	}

	// Truncate the string in place
	*end = '\0';
}

/**
 * @brief Keeps only the last N characters of a UTF-8 string in place.
 *
 * @param str Input UTF-8 string (modified in place).
 * @param max_chars Maximum number of characters to keep from the end.
 */
static void utf8_keep_last_chars(
	char   *str,
	size_t max_chars
){
	if(!str || max_chars == 0)
	{
		return;
	}

	size_t len = strlen(str);
	unsigned char *ptr = (unsigned char *)str + len;
	size_t count = 0;

	// Move backwards through the string character by character
	while(ptr > (unsigned char *)str)
	{
		unsigned char *prev = ptr - 1;

		while(prev > (unsigned char *)str && (*prev & 0xC0) == 0x80)
		{
			prev--; // Move to the start of the current UTF-8 character
		}

		if(++count == max_chars)
		{
			ptr = prev;
			break;
		}

		ptr = prev;
	}

	// Ensure valid UTF-8 starting position
	while((*ptr & 0xC0) == 0x80)
	{
		ptr++;
	}

	// Move the last N characters to the beginning
	size_t remaining_bytes = strlen((char *)ptr);
	memmove(str,ptr,remaining_bytes + 1);
}

/**
 * @brief Shortens a UTF-8 path string if it exceeds maximum length.
 *
 * If the path length exceeds maxLen characters, the function truncates
 * the middle portion of the string and inserts a Unicode ellipsis.
 *
 * @param path[in,out] Path string to be shortened
 *
 * @return Return status code:
 *         - SUCCESS: Path was shortened successfully or didn't need shortening
 *         - FAILURE: Memory operation failed
 */
Return shorten_path(char *path){

	Return status = SUCCESS;

	/* Validate input parameters */
	if(NULL == path)
	{
		/* No action needed */
		return(status);
	}

	// Convert UTF-8 string to wide-character string to measure its length correctly
	size_t len = mbstowcs(NULL,path,0);

	if(len == (size_t)-1 || len == (size_t)-2)
	{
		/* Invalid UTF-8 encoding. No action needed */
		return(status);
	}

	if(len < 2)
	{
		/* No action needed */
		return(status);
	}

	size_t maxLen = terminal_width();

	/* Integer overflow possible if this value is less than threshold */
	if(maxLen < 8)
	{
		/* No action needed */
		return(status);
	}

	/* Path is within limits, no action needed */
	if(len <= maxLen)
	{
		/* No action needed */
		return(status);
	}

	const char *ellipsis = "…"; // Unicode ellipsis (U+2026)
	mbstate_t state = {0}; // Initialize multibyte state
	size_t ellipsis_length = mbrlen(ellipsis,MB_CUR_MAX,&state);

	if(ellipsis_length == (size_t)-1 || ellipsis_length == (size_t)-2)
	{
		/* Invalid UTF-8 encoding. No action needed */
		return(status);
	}

	size_t startLen = (maxLen / 2) - ellipsis_length;
	size_t endLen = maxLen - startLen - ellipsis_length;

	char *start = (char *)malloc(startLen + 1);
	char *end = (char *)malloc(endLen + 1);

	if(NULL == start || NULL == end)
	{
		report("Memory allocation failed");
		status = FAILURE;
	}

	if(SUCCESS == status && startLen > 0 && endLen > 0)
	{
		/* Copy path parts */
		strncpy(start,path,startLen);
		start[startLen] = '\0';
		utf8_truncate(start,startLen);
		remove_trailing_dots(start);

		strncpy(end,path + strlen(path) - endLen,endLen);
		end[endLen] = '\0';
		utf8_keep_last_chars(end,endLen);
		remove_leading_dots(end);
	}

	/* Allocate memory for result */
	size_t result_size = maxLen + 1;
	char *result = (char *)malloc(result_size);

	if(result == NULL)
	{
		report("Memory allocation failed");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Format shortened path */
		if(snprintf(result,result_size,"%s%s%s",start,ellipsis,end) < 0)
		{
			report("snprintf() failed");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		// Copy to path safely
		strncpy(path,result,result_size);
		path[maxLen] = '\0';
	}

	/* Cleanup */
	free(start);
	free(end);
	free(result);

	return(status);
}

