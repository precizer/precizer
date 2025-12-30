#include "testitall.h"
#include <errno.h>
#include <time.h>

/**
 * @brief Create a unique temporary directory under /tmp and return its path.
 *
 * Creates /tmp/precizer.XXXXXXXXXXXXXXXXXX where X are random characters
 * from [a-zA-Z0-9]. The resulting absolute path is written into the
 * caller-provided buffer.
 *
 * @param path Destination buffer (e.g., char path[PATH_MAX] = {0};).
 * @param path_size Size of the destination buffer in bytes (e.g., sizeof(path)).
 * @return SUCCESS on success, FAILURE on error or insufficient space.
 */
Return create_tmpdir(
	char   *path,
	size_t path_size)
{
	static bool seeded = false;
	const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	const size_t charset_len = strlen(charset);
	const size_t suffix_len = 18U; // Number of random characters after the prefix
	const char *prefix = "/tmp/precizer.";
	const size_t prefix_len = strlen(prefix);

	if(NULL == path || 0U == path_size)
	{
		return(FAILURE);
	}

	if(prefix_len + suffix_len + 1U > path_size)
	{
		path[0] = '\0';
		return(FAILURE);
	}

	if(false == seeded)
	{
		struct timespec ts;

		if(0 == clock_gettime(CLOCK_REALTIME,&ts))
		{
			srand((unsigned int)(ts.tv_nsec ^ ts.tv_sec ^ (unsigned int)getpid()));
		} else {
			srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
		}
		seeded = true;
	}

	for(int attempt = 0; attempt < 16; attempt++)
	{
		memcpy(path,prefix,prefix_len);

		for(size_t i = 0U; i < suffix_len; i++)
		{
			path[prefix_len + i] = charset[(size_t)(rand() % (int)charset_len)];
		}

		path[prefix_len + suffix_len] = '\0';

		if(0 == mkdir(path,0700))
		{
			return(SUCCESS);
		}

		if(EEXIST != errno)
		{
			path[0] = '\0';
			return(FAILURE);
		}
	}

	path[0] = '\0';
	return(FAILURE);
}
