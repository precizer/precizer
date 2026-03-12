#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <stdarg.h>

#include "testitall.h"

/**
 * @brief Match managed memory buffers against a PCRE2 pattern.
 *
 * @param text Text descriptor that stores subject data.
 * @param pattern Pattern descriptor interpreted as PCRE2 expression.
 * @return SUCCESS on match, FAILURE otherwise.
 */
Return match_pattern(
	const memory *text,
	const memory *pattern,
	...)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *text_view = NULL;
	const char *pattern_view = NULL;
	const char *filename = NULL;
#if 0
	char *diff = NULL;
#endif

	if((text == NULL) || (pattern == NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Acquire null-terminated views guarded by libmem */
		text_view = getcstring(text);
		pattern_view = getcstring(pattern);

		if((text_view == NULL) || (pattern_view == NULL))
		{
			status = FAILURE;
		}
	}

	va_list args;
	va_start(args,pattern);

	/* Optional third argument carries template filename for diagnostics */
	filename = va_arg(args,const char *);
	va_end(args);

	/* Compile the regular expression */
	pcre2_code *compiled_pattern = NULL;
	pcre2_match_data *match_data = NULL;

	if(SUCCESS == status)
	{
		int errornumber = 0;
		PCRE2_SIZE erroroffset = 0;

		/* Compile the regular expression with multiline and dot-all flags */
		compiled_pattern = pcre2_compile(
			(PCRE2_SPTR)pattern_view,
			PCRE2_ZERO_TERMINATED,
			PCRE2_MULTILINE | PCRE2_DOTALL, /* Enable multiline, extended mode and dot matching newline */
			&errornumber,
			&erroroffset,
			NULL);

		if(compiled_pattern == NULL)
		{
			PCRE2_UCHAR buffer[MAX_CHARACTERS];
			pcre2_get_error_message(errornumber,buffer,sizeof(buffer));
			echo(STDERR,"ERROR: Regex compilation error at offset %d: %s\n",
				(int)erroroffset,buffer);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Allocate memory for match data */
		match_data = pcre2_match_data_create_from_pattern(compiled_pattern,NULL);

		if(match_data == NULL)
		{
			echo(STDERR,"ERROR: Failed to allocate PCRE2 match data\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Attempt to match */
		int match_result = pcre2_match(
			compiled_pattern,
			(PCRE2_SPTR)text_view,
			PCRE2_ZERO_TERMINATED,
			0,
			0,
			match_data,
			NULL);

		if(match_result < 0)
		{
			/* Surface mismatch context or raw PCRE error */
			if(match_result == PCRE2_ERROR_NOMATCH)
			{
#if 0
				/* Get diff */
				status = compare_strings(&diff,text_view,pattern_view);
#endif

#if 0
				{
					// Sequentially match increasing portions of the subject string
					size_t mismatch_offset = 0;
					size_t subject_length = strlen(text_view);

					for(size_t i = 1; i <= subject_length; i++)
					{
						match_result = pcre2_match(
							compiled_pattern,
							(PCRE2_SPTR)text_view,
							i,
							0,
							0,
							match_data,
							NULL);

						if(match_result < 0)
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

				if(filename != NULL)
				{
					echo(STDERR,YELLOW "ERROR: The pattern not match!" RESET "\n"
#if 0
						YELLOW "Diff:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
#endif
						YELLOW "Output:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
						YELLOW "Compared to a pattern from the file %s:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
#if 0
						diff,
#endif
						text_view,filename,pattern_view);
				} else {
					echo(STDERR,YELLOW "ERROR: The pattern not match!" RESET "\n"
#if 0
						YELLOW "Diff:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
#endif
						YELLOW "Output:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
						YELLOW "Compared to a pattern:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
#if 0
						diff,
#endif
						text_view,pattern_view);
				}
			} else {
				echo(STDERR,YELLOW "ERROR: pcre2_match error: %d" RESET "\n",match_result);
			}

			status = FAILURE;
		}
	}

	if(match_data != NULL)
	{
		pcre2_match_data_free(match_data);
	}

	if(compiled_pattern != NULL)
	{
		pcre2_code_free(compiled_pattern);
	}

#if 0
	/* Release diff buffer produced by optional comparison helpers */
	free(diff);
#endif

	deliver(status);
}
