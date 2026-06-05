#include "mocks.h"
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
static bool mock_access_enabled = false;
static size_t mock_access_calls = 0;
static const char *mock_access_target_suffix = NULL;
static int mock_access_errno = EIO;

/**
 * @brief Check whether a path is exactly a suffix or ends with it as a path component
 *
 * @param[in] path Candidate path passed to a wrapped libc call
 * @param[in] suffix Target path suffix configured by a test
 * @return true when @p path matches @p suffix directly or after a slash boundary
 */
static bool mock_path_matches_suffix(
	const char *path,
	const char *suffix)
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

/**
 * @brief Select the path suffix whose opened stream should receive a mocked fread error
 *
 * @param[in] suffix Target suffix, or NULL to clear it
 */
void mocks_fread_set_target_suffix(const char *suffix)
{
	mock_fread_target_suffix = suffix;
}

/**
 * @brief Enable or disable the mocked fread failure
 *
 * @param[in] enabled true to make the first read from the tracked stream fail
 */
void mocks_fread_enable(bool enabled)
{
	mock_fread_enabled = enabled;
}

/**
 * @brief Reset all fread mock state to its defaults
 */
void mocks_fread_reset(void)
{
	mock_fread_enabled = false;
	mock_fread_calls = 0;
	mock_fread_target_suffix = NULL;
	mock_fread_target_stream = NULL;
	mock_fread_error_seen = false;
	mock_fread_errno = EIO;
}

/**
 * @brief Return the number of fread calls intercepted by the mock failure path
 *
 * @return Count of forced fread failures
 */
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

/**
 * @brief Select the path suffix that should receive a mocked remove failure
 *
 * @param[in] suffix Target suffix, or NULL to clear it
 */
void mocks_remove_set_target_suffix(const char *suffix)
{
	mock_remove_target_suffix = suffix;
}

/**
 * @brief Enable or disable the mocked remove failure
 *
 * @param[in] enabled true to make the first matching remove call fail
 */
void mocks_remove_enable(bool enabled)
{
	mock_remove_enabled = enabled;
}

/**
 * @brief Reset all remove mock state to its defaults
 */
void mocks_remove_reset(void)
{
	mock_remove_enabled = false;
	mock_remove_calls = 0;
	mock_remove_target_suffix = NULL;
	mock_remove_errno = EACCES;
}

/**
 * @brief Return the number of remove calls intercepted by the mock failure path
 *
 * @return Count of forced remove failures
 */
size_t mocks_remove_call_count(void)
{
	return mock_remove_calls;
}

/**
 * @brief Set errno returned by the simulated remove failure path
 *
 * @param[in] err Errno value to expose after the forced remove failure
 */
void mocks_remove_set_errno(int err)
{
	mock_remove_errno = err;
}

/**
 * @brief Select the path suffix that should receive a mocked access failure
 *
 * @param[in] suffix Target suffix, or NULL to clear it
 */
void mocks_access_set_target_suffix(const char *suffix)
{
	mock_access_target_suffix = suffix;
}

/**
 * @brief Enable or disable the mocked access failure
 *
 * @param[in] enabled true to make matching access calls fail
 */
void mocks_access_enable(bool enabled)
{
	mock_access_enabled = enabled;
}

/**
 * @brief Reset all access mock state to its defaults
 */
void mocks_access_reset(void)
{
	mock_access_enabled = false;
	mock_access_calls = 0;
	mock_access_target_suffix = NULL;
	mock_access_errno = EIO;
}

/**
 * @brief Return the number of access calls intercepted by the mock failure path
 *
 * @return Count of forced access failures
 */
size_t mocks_access_call_count(void)
{
	return mock_access_calls;
}

/**
 * @brief Set errno returned by the simulated access failure path
 *
 * @param[in] err Errno value to expose after the forced access failure
 */
void mocks_access_set_errno(int err)
{
	mock_access_errno = err;
}

FILE *__real_fopen(
	const char *path,
	const char *mode);

/**
 * @brief Track the stream for the configured fread target while delegating fopen
 *
 * @param[in] path Path passed to fopen
 * @param[in] mode Mode passed to fopen
 * @return Real fopen result
 */
FILE *__wrap_fopen(
	const char *path,
	const char *mode)
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

size_t __real_fread(
	void   *ptr,
	size_t size,
	size_t nmemb,
	FILE   *stream);

/**
 * @brief Force one fread failure on the tracked stream when the mock is enabled
 *
 * @param[out] ptr Destination buffer passed to fread
 * @param[in] size Element size passed to fread
 * @param[in] nmemb Element count passed to fread
 * @param[in] stream Stream passed to fread
 * @return Zero for the forced failure or the real fread result otherwise
 */
size_t __wrap_fread(
	void   *ptr,
	size_t size,
	size_t nmemb,
	FILE   *stream)
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

/**
 * @brief Force one remove failure for the configured target suffix
 *
 * @param[in] path Path passed to remove
 * @return -1 for the forced failure or the real remove result otherwise
 */
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

int __real_access(
	const char *path,
	int        mode);

/**
 * @brief Force access failure for paths matching the configured target suffix
 *
 * @param[in] path Path passed to access
 * @param[in] mode Access mode passed to access
 * @return -1 for the forced failure or the real access result otherwise
 */
int __wrap_access(
	const char *path,
	int        mode)
{
	if(mock_access_enabled
	        && path != NULL
	        && mock_access_target_suffix != NULL
	        && mock_path_matches_suffix(path,mock_access_target_suffix))
	{
		mock_access_calls++;
		errno = mock_access_errno;
		return -1;
	}

	return __real_access(path,mode);
}
