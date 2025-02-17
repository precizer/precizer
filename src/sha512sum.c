#include "precizer.h"

/**
 * @brief Determines the maximum amount of memory that can be allocated for the buffer.
 *
 * This function estimates how much memory can be allocated for a buffer based on
 * available physical memory. It defaults to 1MB if system calls fail.
 *
 * @note The function assumes that only 1% of available RAM should be used for the buffer.
 *       It may not be suitable for embedded or IoT devices with constrained memory.
 *
 * @return The maximum buffer size in bytes. Defaults to 1MB if system information is unavailable.
 */
static long how_much_memory_can_be_allocated_for_the_buffer(void)
{
	// Default value is 1MB buffer. Is it too big for embedded and IoT?
	const long buffer_size = 1024*1024;

	long pages = sysconf(_SC_AVPHYS_PAGES); // Number of actually free pages
	long page_size = sysconf(_SC_PAGESIZE); // Page size in bytes

	if(pages == -1 || page_size == -1)
	{
		return(buffer_size);
	}

	// Only 1% of available RAM
	long available_memory = pages * page_size;
	long one_percent = available_memory / 100;
	return(one_percent);
}

/**
 *
 * Calculate SHA512 cryptographic hash of a file
 *
 */
Return sha512sum(
	const char               *path,
	const short unsigned int *path_size,
	unsigned char            *sha512,
	sqlite3_int64            *offset,
	SHA512_Context           *mdContext)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	const long buffer_size = how_much_memory_can_be_allocated_for_the_buffer();

	if(buffer_size <= 0)
	{
		slog(ERROR,"Invalid buffer size: %ld bytes\n",buffer_size);
		status = FAILURE;
		provide(status);
	}

	unsigned char *buffer = (unsigned char *)calloc((size_t)buffer_size,sizeof(unsigned char));

	if(buffer == NULL)
	{
		report("Memory allocation failed, requested size: %zu bytes",(size_t)buffer_size * sizeof(unsigned char));
		status = FAILURE;
		provide(status);
	}

	char *absolute_path = NULL;
	size_t len = 0;

	FILE *fileptr = fopen(path,"rb");

	if(fileptr == NULL)
	{
		// No read permission
		if(errno == EACCES)
		{
			free(buffer);
			provide(status);
		}

		status = path_absolute_from_relative(&absolute_path,path,path_size);

		if(absolute_path == NULL || SUCCESS != status)
		{
			slog(ERROR,"Can't constructs an absolute path from the base directory %s and a relative path %s\n",config->running_dir,path);

			if(absolute_path != NULL)
			{
				free(absolute_path);
			}
			free(buffer);
			provide(status);
		}

		fileptr = fopen(absolute_path,"rb");

		if(fileptr == NULL)
		{
			// No read permission
			if(errno == EACCES)
			{
				free(buffer);
				free(absolute_path);
				provide(status);
			}

			slog(ERROR,"Can open the file using neither relative %s nor absolute %s path with errno: %d\n",path,absolute_path,errno);
			status = FAILURE;
			free(buffer);
			free(absolute_path);
			provide(status);
		}
	}

	// It moves the file pointer "offset" bytes from the beginning of the file
	if(fseek(fileptr,*offset,SEEK_SET) != 0)
	{
		slog(ERROR,"Failed to seek to offset %lld in file %s\n",*offset,path);
		free(buffer);
		free(absolute_path);
		fclose(fileptr);
		status = FAILURE;
		provide(status);
	}

	bool loop_was_interrupted = false;

	if(*offset == 0)
	{
		if(sha512_init(mdContext) == 1)
		{
			slog(ERROR,"SHA512 initialization failed\n");
			free(buffer);
			free(absolute_path);
			fclose(fileptr);
			status = FAILURE;
			provide(status);
		}
	}

	while((len = fread(buffer,1,(size_t)buffer_size,fileptr)) != 0)
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true)
		{
			loop_was_interrupted = true;
			break;
		}

		if(ferror(fileptr))
		{
			slog(ERROR,"Error reading file %s\n",path);
			status = FAILURE;
			break;
		}

		if(SUCCESS == status)
		{
			if(sha512_update(mdContext,buffer,len) == 1)
			{
				slog(ERROR,"SHA512 update failed\n");
				status = FAILURE;
				break;
			}

			*offset += (sqlite3_int64)len;
		}
	}

	free(buffer);

	if(fclose(fileptr) != 0)
	{
		slog(ERROR,"Error closing file %s\n",path);
	}

	free(absolute_path);

	if(SUCCESS == status)
	{
		if(loop_was_interrupted == false)
		{
			*offset = 0;

			if(sha512_final(mdContext,sha512) == 1)
			{
				slog(ERROR,"SHA512 finalization failed\n");
				status = FAILURE;
			}
		}
	}

#if 0

	for(size_t i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		printf("%02x",sha512[i]);
	}
	putchar('\n');

#endif

	provide(status);
}
