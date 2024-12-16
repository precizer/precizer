#include "testitall.h"

// Function to replace a placeholder in the string
Return replace_placeholder(
	char **pattern,
	const char *placeholder,
	const char *replacement
){
	if (!pattern || !*pattern || !placeholder || !replacement) {
		return(FAILURE); // Invalid arguments
	}

	pcre2_code *re;
	PCRE2_SPTR pattern_str = (PCRE2_SPTR)placeholder;
	PCRE2_SPTR subject = (PCRE2_SPTR)(*pattern);
	PCRE2_SIZE subject_len = strlen((const char *)subject);
	PCRE2_SPTR replacement_str = (PCRE2_SPTR)replacement;

	int err_code;
	PCRE2_SIZE err_offset;

	// Compile the regular expression
	re = pcre2_compile(
		pattern_str,
		PCRE2_ZERO_TERMINATED,
		0,
		&err_code,
		&err_offset,
		NULL
	);

	if (re == NULL) {
		PCRE2_UCHAR err_msg[256];
		pcre2_get_error_message(err_code, err_msg, sizeof(err_msg));
		echo(STDERR,"ERROR: PCRE2 compilation failed at offset %d: %s\n", (int)err_offset, err_msg);
		return(FAILURE);
	}

	pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

	// Estimate output buffer size based on the number of matches and replacement size
	PCRE2_SIZE out_len = subject_len + 1; // Start with original length
	size_t match_count = 0;
	int rc = pcre2_match(re, subject, subject_len, 0, 0, match_data, NULL);
	while (rc >= 0)
	{
		out_len += strlen(replacement) - (pcre2_get_ovector_pointer(match_data)[1] -
		                                   pcre2_get_ovector_pointer(match_data)[0]);
		match_count++;
		rc = pcre2_match(re, subject, subject_len,
		             pcre2_get_ovector_pointer(match_data)[1], 0, match_data, NULL);
	}

	// Allocate buffer for the output string
	char *output = malloc(out_len);
	if (!output) {
		pcre2_code_free(re);
		pcre2_match_data_free(match_data);
		report("Memory allocation failed, requested size: %zu bytes", out_len);
		return(FAILURE); // Memory allocation failure
	}

	// Perform the substitution
	PCRE2_SIZE out_len_actual = out_len;
	rc = pcre2_substitute(re, subject, subject_len, 0, PCRE2_SUBSTITUTE_GLOBAL,
	                  match_data, NULL, replacement_str, PCRE2_ZERO_TERMINATED,
	                  (PCRE2_UCHAR8 *)output, &out_len_actual);

	if (rc < 0) {
		echo(STDERR,"ERROR: PCRE2 substitution error: %d\n", rc);
		free(output);
		pcre2_code_free(re);
		pcre2_match_data_free(match_data);
		return(FAILURE);
	}

	// Free the old pattern and update the pointer
	free(*pattern);
	*pattern = output;

	pcre2_code_free(re);
	pcre2_match_data_free(match_data);
	return(SUCCESS);
}
