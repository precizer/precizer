#include "mocks.h"
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static bool mock_fread_enabled = false;
static size_t mock_fread_calls = 0;
static const char *mock_fread_target_suffix = NULL;
static FILE *mock_fread_target_stream = NULL;
static bool mock_fread_error_seen = false;
static int mock_fread_errno = EIO;

static bool mock_path_matches_suffix(const char *path,const char *suffix)
{
	size_t path_len = 0;
	size_t suffix_len = 0;

	if(path == NULL || suffix == NULL)
	{
		return false;
	}

	if(strcmp(path,suffix) == 0)
	{
		return true;
	}

	path_len = strlen(path);
	suffix_len = strlen(suffix);

	if(path_len < suffix_len + 1U)
	{
		return false;
	}

	if(path[path_len - suffix_len - 1U] != '/')
	{
		return false;
	}

	return strcmp(path + (path_len - suffix_len),suffix) == 0;
}

void mocks_fread_set_target_suffix(const char *suffix)
{
	mock_fread_target_suffix = suffix;
}

void mocks_fread_enable(bool enabled)
{
	mock_fread_enabled = enabled;
}

void mocks_fread_reset(void)
{
	mock_fread_enabled = false;
	mock_fread_calls = 0;
	mock_fread_target_suffix = NULL;
	mock_fread_target_stream = NULL;
	mock_fread_error_seen = false;
	mock_fread_errno = EIO;
}

size_t mocks_fread_call_count(void)
{
	return mock_fread_calls;
}

void mocks_fread_set_errno(int err)
{
	mock_fread_errno = err;
}

FILE *__real_fopen(const char *path,const char *mode);

FILE *__wrap_fopen(const char *path,const char *mode)
{
	FILE *stream = __real_fopen(path,mode);

	if(stream != NULL && mock_fread_target_suffix != NULL)
	{
		if(mock_path_matches_suffix(path,mock_fread_target_suffix))
		{
			mock_fread_target_stream = stream;
		}
	}

	return stream;
}

size_t __real_fread(void *ptr,size_t size,size_t nmemb,FILE *stream);

size_t __wrap_fread(void *ptr,size_t size,size_t nmemb,FILE *stream)
{
	if(mock_fread_enabled
	        && stream != NULL
	        && stream == mock_fread_target_stream
	        && mock_fread_calls == 0)
	{
		mock_fread_calls++;
		mock_fread_error_seen = true;
		errno = mock_fread_errno;
		return 0;
	}

	return __real_fread(ptr,size,nmemb,stream);
}

int __real_ferror(FILE *stream);

int __wrap_ferror(FILE *stream)
{
	if(stream != NULL && stream == mock_fread_target_stream && mock_fread_error_seen)
	{
		return 1;
	}

	return __real_ferror(stream);
}
