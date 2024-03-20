#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include "precizer.h"
#include "pcre2.h"

REGEXP regexp_match
(
	const char *regexp,
	const char *relative_path,
	bool *showed_once
){
	bool match = false;

	/* for pcre2_compile */
	pcre2_code *re;
	PCRE2_SIZE erroffset;
	int errcode;
	PCRE2_UCHAR8 buffer[127];

	/* for pcre2_match */
	int rc;
#if 0
	PCRE2_SIZE* ovector;
#endif

	const unsigned char *pattern = (const unsigned char*)regexp;
	size_t pattern_size = strlen(regexp);

	const unsigned char *subject = (const unsigned char*)relative_path;
	size_t subject_size = strlen(relative_path);
	uint32_t options = 0;

	pcre2_match_data *match_data;
	uint32_t ovecsize = 128;

	re = pcre2_compile(pattern, pattern_size, options, &errcode, &erroffset, NULL);
	if (re == NULL)
	{
		if(*showed_once == false)
		{
			*showed_once = true;
			pcre2_get_error_message(errcode, buffer, 127);
			slog(false,"PCRE2 regular expression %s has an error:%d %s\n",pattern,errcode,buffer);
		}
		return(REGEXP_ERROR);
	}

	match_data = pcre2_match_data_create(ovecsize, NULL);
	rc = pcre2_match(re, subject, subject_size, 0, options, match_data, NULL);
	if(rc == 0)
	{
		if(*showed_once == false)
		{
			*showed_once = true;
			pcre2_get_error_message(errcode, buffer, 127);
			slog(false,"PCRE2 regular expression %s has an error: %d \"offset vector too small\"\n",pattern,rc);
		}
		return(REGEXP_ERROR);
	}
	else if(rc > 0)
	{
#if 0
		ovector = pcre2_get_ovector_pointer(match_data);
#endif

		for(PCRE2_SIZE i = 0; i < (size_t)rc; i++)
		{
#if 0
			PCRE2_SPTR start = subject + ovector[2*i];
			PCRE2_SIZE slen = ovector[2*i+1] - ovector[2*i];
			printf( "%2ld: %.*s\n", i, (int)slen, (const char *)start );
#endif
			match = true;
		}
	}
#if 0
	else if (rc < 0)
	{
		printf("No match\n");
	}
#endif

	pcre2_match_data_free(match_data);
	pcre2_code_free(re);

	return(match);
}
