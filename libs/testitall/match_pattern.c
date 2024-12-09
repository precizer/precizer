#include "testitall.h"

/**
 * @brief Matches text against a regular expression pattern with detailed error reporting
 *
 * @param text The input text to match against
 * @param pattern The regular expression pattern to use
 * @return Return SUCCESS if match is found, FAILURE otherwise
 */
Return match_pattern(
	const char *text,
	const char *pattern
){
	Return status = SUCCESS;

	// Compile the regular expression
	pcre2_code *re;
	int errornumber;
	PCRE2_SIZE erroroffset;
	pcre2_match_data *match_data;

	/* Compile the regular expression with multiline and dot-all flags */
	re = pcre2_compile(
		(PCRE2_SPTR)pattern,
		PCRE2_ZERO_TERMINATED,
		PCRE2_MULTILINE | PCRE2_DOTALL, /* Enable multiline, extended mode and dot matching newline */
		&errornumber,
		&erroroffset,
		NULL
	);

	if (re == NULL)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
		echo(STDERR, "ERROR: Regex compilation error at offset %d: %s\n",
			(int)erroroffset, buffer);
		return(FAILURE);
	}

	/* Allocate memory for match data */
	match_data = pcre2_match_data_create_from_pattern(re, NULL);

	/* Attempt to match */
	int rc = pcre2_match(
		re,
		(PCRE2_SPTR)text,
		PCRE2_ZERO_TERMINATED,
		0,
		0,
		match_data,
		NULL
	);

	if (rc < 0)
	{
		switch(rc)
		{
			case PCRE2_ERROR_NOMATCH:
#if 0
			{
				/* Find first mismatch position and context */
				size_t pos = 0;
				size_t context_size = 50; /* Show this many characters around mismatch */
				int text_len = strlen(text);

				while (pos < text_len)
				{
					/* Try matching from current position */
					rc = pcre2_match(
						re,
						(PCRE2_SPTR)(text + pos),
						text_len - pos,
						0,
						0,
						match_data,
						NULL
					);

					if (rc >= 0)
					{
						break;
					}

					/* Calculate context range */
					size_t start = (pos > context_size) ? pos - context_size : 0;
					size_t end = (pos + context_size < text_len) ? pos + context_size : text_len;

					/* Get the character at mismatch position */
					char mismatch_char = text[pos];

					echo(STDERR,
						"ERROR: Pattern not match at position: %zu\n"
						"Character at mismatch: '%c' (ASCII: %d)\n"
						"Context around mismatch:\n"
						YELLOW ">>" RESET "%.*s" RED "%c" RESET "%.*s" YELLOW "<<\n" RESET,
						pos,
						mismatch_char, (int)mismatch_char,
						(int)(pos - start), text + start,
						mismatch_char,
						(int)(end - pos - 1), text + pos + 1);

					echo(STDERR,
						"Text:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n" YELLOW \
						"Compared to a pattern:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n" RESET,
						text, pattern);

					status = FAILURE;
					break;
				}
				break;
			}
#else
				echo(STDERR, "ERROR: The pattern not match!\n" \
					"Text:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n" \
					YELLOW "Compared to a pattern:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n" RESET,
					text, pattern);
				status = FAILURE;
				break;
#endif
			default:
				echo(STDERR, "ERROR: pcre2_match error: %d\n", rc);
				status = FAILURE;
				break;
		}
	}

	/* Cleanup */
	pcre2_match_data_free(match_data);
	pcre2_code_free(re);

	return(status);
}
