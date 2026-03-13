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
static bool mock_remove_enabled = false;
static size_t mock_remove_calls = 0;
static const char *mock_remove_target_suffix = NULL;
static int mock_remove_errno = EACCES;

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

/**
 * @brief Set errno returned by the simulated fread failure path
 *
 * @param[in] err Errno value to expose after the forced read failure
 */
void mocks_fread_set_errno(int err)
{
	mock_fread_errno = err;
}

void mocks_remove_set_target_suffix(const char *suffix)
{
	mock_remove_target_suffix = suffix;
}

void mocks_remove_enable(bool enabled)
{
	mock_remove_enabled = enabled;
}

void mocks_remove_reset(void)
{
	mock_remove_enabled = false;
	mock_remove_calls = 0;
	mock_remove_target_suffix = NULL;
	mock_remove_errno = EACCES;
}

size_t mocks_remove_call_count(void)
{
	return mock_remove_calls;
}

void mocks_remove_set_errno(int err)
{
	mock_remove_errno = err;
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

/**
 * @brief Report a pending mocked fread error exactly once for the tracked stream
 *
 * @param[in] stream Stream passed to `ferror()`
 * @return `1` for the pending mocked error or the real `ferror()` result otherwise
 */
int __wrap_ferror(FILE *stream)
{
	if(stream != NULL && stream == mock_fread_target_stream && mock_fread_error_seen)
	{
		/*
		 * Clear the target once the error is observed to avoid matching a
		 * recycled FILE* address for unrelated files (seen in coverage builds).
		 */
		mock_fread_target_stream = NULL;
		mock_fread_error_seen = false;
		return 1;
	}

	return __real_ferror(stream);
}

int __real_remove(const char *path);

int __wrap_remove(const char *path)
{
	if(mock_remove_enabled
	        && path != NULL
	        && mock_remove_target_suffix != NULL
	        && mock_remove_calls == 0
	        && mock_path_matches_suffix(path,mock_remove_target_suffix))
	{
		mock_remove_calls++;
		errno = mock_remove_errno;
		return -1;
	}

	return __real_remove(path);
}
