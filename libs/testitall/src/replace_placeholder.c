#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "testitall.h"

Return replace_placeholder(
	memory     *pattern,
	const char *placeholder,
	const char *replacement)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if((pattern == NULL) || (placeholder == NULL) || (replacement == NULL))
	{
		return FAILURE;
	}

	const char *subject = getcstring(pattern);

	if(subject == NULL)
	{
		return FAILURE;
	}

	const size_t subject_len = strlen(subject);

	pcre2_code *re = NULL;
	pcre2_match_data *match_data = NULL;
	create(char,replacement_buffer);

	const PCRE2_SPTR pattern_str = (PCRE2_SPTR)placeholder;
	const PCRE2_SPTR subject_str = (PCRE2_SPTR)subject;
	const PCRE2_SPTR replacement_str = (PCRE2_SPTR)replacement;

	int err_code = 0;
	PCRE2_SIZE err_offset = 0;

	re = pcre2_compile(
		pattern_str,
		PCRE2_ZERO_TERMINATED,
		0,
		&err_code,
		&err_offset,
		NULL
	);

	if(re == NULL)
	{
		PCRE2_UCHAR err_msg[MAX_CHARACTERS];
		pcre2_get_error_message(err_code,err_msg,sizeof(err_msg));
		echo(STDERR,"ERROR: PCRE2 compilation failed at offset %d: %s\n",(int)err_offset,err_msg);
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		match_data = pcre2_match_data_create_from_pattern(re,NULL);

		if(match_data == NULL)
		{
			echo(STDERR,"ERROR: Failed to allocate match data\n");
			status = FAILURE;
		}
	}

	PCRE2_SIZE out_len = subject_len + 1;

	if(SUCCESS == status)
	{
		int rc = pcre2_match(re,subject_str,subject_len,0,0,match_data,NULL);

		while(rc >= 0)
		{
			out_len += strlen(replacement) - (pcre2_get_ovector_pointer(match_data)[1] -
			        pcre2_get_ovector_pointer(match_data)[0]);
			rc = pcre2_match(re,subject_str,subject_len,
				pcre2_get_ovector_pointer(match_data)[1],0,match_data,NULL);
		}

		if(out_len == 0)
		{
			out_len = 1;
		}

		status = resize(replacement_buffer,(size_t)out_len);
	}

	PCRE2_SIZE out_len_actual = out_len;

	if(SUCCESS == status)
	{
		char *output = data(char,replacement_buffer);

		if(output == NULL)
		{
			status = FAILURE;
		} else {
			int rc = pcre2_substitute(
				re,
				subject_str,
				subject_len,
				0,
				PCRE2_SUBSTITUTE_GLOBAL,
				match_data,
				NULL,
				replacement_str,
				PCRE2_ZERO_TERMINATED,
				(PCRE2_UCHAR8 *)output,
				&out_len_actual);

			if(rc < 0)
			{
				echo(STDERR,"ERROR: PCRE2 substitution error: %d\n",rc);
				status = FAILURE;
			} else {
				const size_t actual_length = (size_t)out_len_actual;

				if(actual_length + 1 > replacement_buffer->length)
				{
					status = resize(replacement_buffer,actual_length + 1);

					output = data(char,replacement_buffer);

					if(output == NULL)
					{
						status = FAILURE;
					}
				}

				if(SUCCESS == status)
				{
					output[actual_length] = '\0';
					status = resize(replacement_buffer,actual_length + 1);
				}
			}
		}
	}

	run(copy(pattern,replacement_buffer));

	call(del(replacement_buffer));

	if(match_data != NULL)
	{
		pcre2_match_data_free(match_data);
	}

	if(re != NULL)
	{
		pcre2_code_free(re);
	}

	deliver(status);
}
