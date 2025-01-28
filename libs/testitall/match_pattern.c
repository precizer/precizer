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
	const char *pattern,
	...
){
	Return status = SUCCESS;
	char *diff = NULL;

	va_list args;
	va_start(args,pattern);

	/* Try to get third argument if it exists */
	const char *filename = va_arg(args,const char *);
	va_end(args);

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

	if(re == NULL)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornumber,buffer,sizeof(buffer));
		echo(STDERR,"ERROR: Regex compilation error at offset %d: %s\n",
			(int)erroroffset,buffer);
		return(FAILURE);
	}

	/* Allocate memory for match data */
	match_data = pcre2_match_data_create_from_pattern(re,NULL);

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

	if(rc < 0)
	{
		switch(rc)
		{
			case PCRE2_ERROR_NOMATCH:
#if 0
				/* Get diff */
				status = compare_strings(&diff,text,pattern);
#endif

#if 0
				{
					// Sequentially match increasing portions of the subject string
					size_t mismatch_offset = 0;
					size_t subject_length = strlen(text);

					for(size_t i = 1; i <= subject_length; i++)
					{
						rc = pcre2_match(re,(PCRE2_SPTR)text,i,0,0,match_data,NULL);

						if(rc < 0)
						{
							mismatch_offset = i - 1; // Last valid match position
							break;
						}
					}

					if(mismatch_offset < subject_length)
					{
						echo(STDERR,"Mismatch starts at offset %zu.\n",mismatch_offset);
					}
				}
#endif

				if(NULL != filename)
				{
					echo(STDERR,"ERROR: The pattern not match!\n"
#if 0
						"Diff:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n"
#endif
						"Text:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n"
						YELLOW "Compared to a pattern from the file %s:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n",
#if 0
						diff,
#endif
						text,filename,pattern);
				} else {
					echo(STDERR,"ERROR: The pattern not match!\n"
#if 0
						"Diff:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n"
#endif
						"Text:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n"
						YELLOW "Compared to a pattern:\n" YELLOW ">>" RESET "%s" YELLOW "<<\n",
#if 0
						diff,
#endif
						text,pattern);
				}
				status = FAILURE;
				break;
			default:
				echo(STDERR,"ERROR: pcre2_match error: %d\n",rc);
				status = FAILURE;
				break;
		}
	}

	/* Cleanup */
	free(diff);
	pcre2_match_data_free(match_data);
	pcre2_code_free(re);

	return(status);
}
