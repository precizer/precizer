#include "testitall.h"
#include <limits.h>
#include <string.h>

/**
 * @brief Write the parent directory of the current working directory into a memory descriptor
 *
 * The current working directory is copied into the provided memory descriptor,
 * trailing slashes are removed, and then the parent directory is kept there
 *
 * Example: if CWD is "/tmp/precizer/run///", the function writes "/tmp/precizer"
 * The root path "/" is preserved as "/"
 *
 * @param path Destination memory descriptor initialized for char elements
 * @return SUCCESS on success, FAILURE on error
 */
Return get_origin_dir(
	memory *path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	char *cwd = NULL;

#if defined(__GLIBC__)
	cwd = get_current_dir_name();
#else
	// Portable fallback for evilOS/BSD.
	cwd = getcwd(NULL,0);
#endif

	if(NULL == cwd)
	{
		del(path);
		deliver(FAILURE);
	}

	if(SUCCESS != copy_cstring(path,cwd,strlen(cwd) + 1U))
	{
		free(cwd);
		del(path);
		deliver(FAILURE);
	}

	free(cwd);

	size_t path_length = 0U;

	if(SUCCESS != string_length(path,&path_length))
	{
		del(path);
		deliver(FAILURE);
	}

	char *path_string = data(char,path);

	if(path_string == NULL)
	{
		del(path);
		deliver(FAILURE);
	}

	/*
	 * Use the visible string length instead of descriptor capacity
	 * Stop at length 1 so "/" does not become an empty string
	 */
	while(path_length > 1U && path_string[path_length - 1U] == '/')
	{
		path_string[--path_length] = '\0';
	}

	char *last_slash = strrchr(path_string,'/');

	if(NULL == last_slash)
	{
		del(path);
		deliver(FAILURE);
	}

	if(last_slash == path_string)
	{
		if(SUCCESS != copy_literal(path,"/"))
		{
			del(path);
			deliver(FAILURE);
		}

		deliver(SUCCESS);
	}

	const size_t parent_length = (size_t)(last_slash - path_string);

	path_string[parent_length] = '\0';

	if(SUCCESS != resize(path,parent_length + 1U,RELEASE_UNUSED))
	{
		del(path);
		deliver(FAILURE);
	}

	deliver(SUCCESS);
}
