#include "mocks.h"
#include <errno.h>
#ifndef EVIL_EMPIRE_OS
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <stdio.h>
#ifndef EVIL_EMPIRE_OS
int fileno(FILE *);
ssize_t readlink(const char *,char *,size_t);
#endif
#include <string.h>

static bool mock_fread_enabled = false;
static size_t mock_fread_calls = 0;
static const char *mock_fread_target_suffix = NULL;
static int mock_fread_errno = EIO;
static bool mock_ferror_enabled = true;
static size_t mock_ferror_calls = 0;
static int mock_ferror_errno = EIO;
static bool mock_openat_enabled = false;
static size_t mock_openat_calls = 0;
static const char *mock_openat_target_suffix = NULL;
static int mock_openat_errno = EIO;
static bool mock_fdopen_enabled = false;
static size_t mock_fdopen_calls = 0;
static const char *mock_fdopen_target_suffix = NULL;
static int mock_fdopen_target_fd = -1;
static int mock_fdopen_errno = EIO;
#ifndef EVIL_EMPIRE_OS
static int mock_fread_target_fd = -1;
static FILE *mock_fread_target_stream = NULL;
static bool mock_fread_zero_seen = false;
#endif
static bool mock_remove_enabled = false;
static size_t mock_remove_calls = 0;
static const char *mock_remove_target_suffix = NULL;
static int mock_remove_errno = EACCES;
static bool mock_access_enabled = false;
static size_t mock_access_calls = 0;
static const char *mock_access_target_suffix = NULL;
static int mock_access_errno = EIO;

#ifndef EVIL_EMPIRE_OS
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
 * @brief Check whether a stream resolves to a configured path suffix
 *
 * @param[in] stream Stream passed to a wrapped stdio call
 * @param[in] suffix Target path suffix configured by a test
 * @return true when @p stream refers to a path matching @p suffix
 */
static bool mock_stream_matches_suffix(
	FILE       *stream,
	const char *suffix)
{
	char descriptor_path[64];
	char resolved_path[4096];
	int stream_fd = -1;
	int descriptor_path_size = 0;
	ssize_t resolved_path_size = 0;

	if(stream == NULL || suffix == NULL)
	{
		return false;
	}

	stream_fd = fileno(stream);

	if(stream_fd < 0)
	{
		return false;
	}

	descriptor_path_size = snprintf(descriptor_path,
		sizeof(descriptor_path),
		"/proc/self/fd/%d",
		stream_fd);

	if(descriptor_path_size < 0 || (size_t)descriptor_path_size >= sizeof(descriptor_path))
	{
		return false;
	}

	resolved_path_size = readlink(descriptor_path,
		resolved_path,
		sizeof(resolved_path) - 1U);

	if(resolved_path_size < 0)
	{
		return false;
	}

	resolved_path[resolved_path_size] = '\0';

	return mock_path_matches_suffix(resolved_path,suffix);
}

/**
 * @brief Check whether a stream is the configured fread target
 *
 * @param[in] stream Stream passed to fread() or ferror()
 * @return true when @p stream matches the current fread target
 */
static bool mock_stream_matches_fread_target(FILE *stream)
{
	if(stream == NULL || mock_fread_target_suffix == NULL)
	{
		return false;
	}

	if(stream == mock_fread_target_stream)
	{
		return true;
	}

	return mock_stream_matches_suffix(stream,mock_fread_target_suffix);
}

/**
 * @brief Check whether openat should fail for a configured target path
 *
 * @param[in] path Path passed to openat()
 * @return true when the openat mock should force an error
 */
static bool mock_should_fail_openat(const char *path)
{
	if(mock_openat_enabled == false)
	{
		return false;
	}

	if(path == NULL || mock_openat_target_suffix == NULL)
	{
		return false;
	}

	if(mock_openat_calls != 0)
	{
		return false;
	}

	return mock_path_matches_suffix(path,mock_openat_target_suffix);
}

/**
 * @brief Track descriptors opened for later fdopen or fread matching
 *
 * @param[in] path Path passed to openat()
 * @param[in] file_descriptor Descriptor returned by the real open call
 */
static void mock_track_opened_descriptor(
	const char *path,
	int         file_descriptor)
{
	if(file_descriptor < 0 || path == NULL)
	{
		return;
	}

	if(mock_fdopen_target_suffix != NULL)
	{
		if(mock_path_matches_suffix(path,mock_fdopen_target_suffix))
		{
			mock_fdopen_target_fd = file_descriptor;
		}
	}

	if(mock_fread_target_suffix != NULL)
	{
		if(mock_path_matches_suffix(path,mock_fread_target_suffix))
		{
			mock_fread_target_fd = file_descriptor;
		}
	}
}
#endif

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
#ifndef EVIL_EMPIRE_OS
	mock_fread_target_fd = -1;
	mock_fread_target_stream = NULL;
	mock_fread_zero_seen = false;
#endif
	mock_fread_errno = EIO;
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Check whether openat() flags require the variadic mode argument
 *
 * @param[in] flags Flags passed to openat()
 * @return true when the caller must provide the file mode argument
 */
static bool mock_open_flags_require_mode(const int flags)
{
	if((flags & O_CREAT) != 0)
	{
		return true;
	}

#ifdef O_TMPFILE
	if((flags & O_TMPFILE) == O_TMPFILE)
	{
		return true;
	}
#endif

	return false;
}

int __real_openat(
	int         directory_fd,
	const char *path,
	int         flags,
	...);

/**
 * @brief Track the descriptor for the configured fread target opened through openat()
 *
 * @param[in] directory_fd Directory descriptor passed to openat()
 * @param[in] path Path passed to openat()
 * @param[in] flags Flags passed to openat()
 * @return Real openat result
 */
int __wrap_openat(
	int         directory_fd,
	const char *path,
	int         flags,
	...)
{
	int file_descriptor = -1;

	if(mock_should_fail_openat(path))
	{
		mock_openat_calls++;
		errno = mock_openat_errno;
		return -1;
	}

	if(mock_open_flags_require_mode(flags) == true)
	{
		va_list arguments;
		va_start(arguments,flags);
		const mode_t mode = va_arg(arguments,mode_t);
		va_end(arguments);

		file_descriptor = __real_openat(directory_fd,path,flags,mode);

	} else {
		file_descriptor = __real_openat(directory_fd,path,flags);
	}

	mock_track_opened_descriptor(path,file_descriptor);

	return file_descriptor;
}

int __real___openat_2(
	int         directory_fd,
	const char *path,
	int         flags);

/**
 * @brief Wrap glibc's checked openat variant used by fortified builds
 *
 * @param[in] directory_fd Directory descriptor passed to __openat_2()
 * @param[in] path Path passed to __openat_2()
 * @param[in] flags Flags passed to __openat_2()
 * @return Mocked failure or real __openat_2 result
 */
int __wrap___openat_2(
	int         directory_fd,
	const char *path,
	int         flags)
{
	int file_descriptor = -1;

	if(mock_should_fail_openat(path))
	{
		mock_openat_calls++;
		errno = mock_openat_errno;
		return -1;
	}

	file_descriptor = __real___openat_2(directory_fd,path,flags);

	mock_track_opened_descriptor(path,file_descriptor);

	return file_descriptor;
}

FILE *__real_fdopen(
	int         file_descriptor,
	const char *mode);

/**
 * @brief Bind the tracked openat() descriptor to the stream later used by fread()
 *
 * @param[in] file_descriptor Descriptor passed to fdopen()
 * @param[in] mode Mode passed to fdopen()
 * @return Real fdopen result
 */
FILE *__wrap_fdopen(
	int         file_descriptor,
	const char *mode)
{
	FILE *stream = NULL;

	if(mock_fdopen_enabled
	        && file_descriptor == mock_fdopen_target_fd
	        && mock_fdopen_calls == 0)
	{
		mock_fdopen_calls++;
		mock_fdopen_target_fd = -1;
		errno = mock_fdopen_errno;
		return NULL;
	}

	stream = __real_fdopen(file_descriptor,mode);

	if(file_descriptor == mock_fread_target_fd)
	{
		if(stream != NULL)
		{
			mock_fread_target_stream = stream;
		}

		mock_fread_target_fd = -1;
	}

	return stream;
}
#endif

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
 * @brief Enable or disable mocked ferror confirmation after a mocked zero read
 *
 * @param[in] enabled true to make ferror report the pending read error
 */
void mocks_ferror_enable(bool enabled)
{
	mock_ferror_enabled = enabled;
}

/**
 * @brief Reset all ferror mock state to its defaults
 */
void mocks_ferror_reset(void)
{
	mock_ferror_enabled = true;
	mock_ferror_calls = 0;
	mock_ferror_errno = EIO;
}

/**
 * @brief Return the number of ferror calls intercepted by the mock path
 *
 * @return Count of mocked ferror confirmations
 */
size_t mocks_ferror_call_count(void)
{
	return mock_ferror_calls;
}

/**
 * @brief Set errno exposed when the mocked ferror path reports an error
 *
 * @param[in] err Errno value to expose with the reported read error
 */
void mocks_ferror_set_errno(int err)
{
	mock_ferror_errno = err;
}

/**
 * @brief Select the path suffix whose openat() call should fail
 *
 * @param[in] suffix Target suffix, or NULL to clear it
 */
void mocks_openat_set_target_suffix(const char *suffix)
{
	mock_openat_target_suffix = suffix;
}

/**
 * @brief Enable or disable the mocked openat failure
 *
 * @param[in] enabled true to make matching openat calls fail
 */
void mocks_openat_enable(bool enabled)
{
	mock_openat_enabled = enabled;
}

/**
 * @brief Reset all openat mock state to its defaults
 */
void mocks_openat_reset(void)
{
	mock_openat_enabled = false;
	mock_openat_calls = 0;
	mock_openat_target_suffix = NULL;
	mock_openat_errno = EIO;
}

/**
 * @brief Return the number of openat calls intercepted by the mock failure path
 *
 * @return Count of forced openat failures
 */
size_t mocks_openat_call_count(void)
{
	return mock_openat_calls;
}

/**
 * @brief Set errno returned by the simulated openat failure path
 *
 * @param[in] err Errno value to expose after the forced open failure
 */
void mocks_openat_set_errno(int err)
{
	mock_openat_errno = err;
}

/**
 * @brief Select the path suffix whose fdopen() call should fail after openat()
 *
 * @param[in] suffix Target suffix, or NULL to clear it
 */
void mocks_fdopen_set_target_suffix(const char *suffix)
{
	mock_fdopen_target_suffix = suffix;
}

/**
 * @brief Enable or disable the mocked fdopen failure
 *
 * @param[in] enabled true to make fdopen fail for the tracked descriptor
 */
void mocks_fdopen_enable(bool enabled)
{
	mock_fdopen_enabled = enabled;
}

/**
 * @brief Reset all fdopen mock state to its defaults
 */
void mocks_fdopen_reset(void)
{
	mock_fdopen_enabled = false;
	mock_fdopen_calls = 0;
	mock_fdopen_target_suffix = NULL;
	mock_fdopen_target_fd = -1;
	mock_fdopen_errno = EIO;
}

/**
 * @brief Return the number of fdopen calls intercepted by the mock failure path
 *
 * @return Count of forced fdopen failures
 */
size_t mocks_fdopen_call_count(void)
{
	return mock_fdopen_calls;
}

/**
 * @brief Set errno returned by the simulated fdopen failure path
 *
 * @param[in] err Errno value to expose after the forced fdopen failure
 */
void mocks_fdopen_set_errno(int err)
{
	mock_fdopen_errno = err;
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

#ifndef EVIL_EMPIRE_OS
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
	        && mock_stream_matches_fread_target(stream)
	        && mock_fread_calls == 0)
	{
		mock_fread_target_stream = stream;
		mock_fread_calls++;
		mock_fread_zero_seen = true;
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
	if(stream != NULL
	        && mock_stream_matches_fread_target(stream)
	        && mock_fread_zero_seen)
	{
		mock_ferror_calls++;
		/*
		 * Clear the target once the zero read is observed to avoid matching a
		 * recycled FILE* address for unrelated files (seen in coverage builds)
		 */
		mock_fread_target_stream = NULL;
		mock_fread_zero_seen = false;

		if(mock_ferror_enabled)
		{
			errno = mock_ferror_errno;
			return 1;
		}

		return 0;
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
#endif
