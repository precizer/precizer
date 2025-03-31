#include "precizer.h"

/**
 *
 * Calculate SHA512 cryptographic hash of a file
 *
 */
Return sha512sum(
	const char               *path,
	const short unsigned int *path_size,
	const long               *buffer_length,
	unsigned char            *sha512,
	sqlite3_int64            *offset,
	SHA512_Context           *mdContext,
	bool                     *wrong_file_type)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(*buffer_length <= 0)
	{
		slog(ERROR,"Invalid buffer size: %ld bytes\n",buffer_length);
		provide(FAILURE);
	}

	size_t buffer_size = (size_t)*buffer_length * sizeof(unsigned char);

	create_mem(mem_uchar,buffer);

	status = calloc_uchar(buffer,buffer_size);

	if(SUCCESS != status)
	{
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
			del_uchar(&buffer);
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
			del_uchar(&buffer);
			provide(status);
		}

		fileptr = fopen(absolute_path,"rb");

		if(fileptr == NULL)
		{
			// No read permission
			if(errno == EACCES)
			{
				del_uchar(&buffer);
				free(absolute_path);
				provide(status);
			}

			slog(ERROR,"Can open the file using neither relative %s nor absolute %s path with errno: %d\n",path,absolute_path,errno);
			del_uchar(&buffer);
			free(absolute_path);
			provide(FAILURE);
		}
	}

	// It moves the file pointer "offset" bytes from the beginning of the file
	if(fseek(fileptr,*offset,SEEK_SET) != 0)
	{
		/* Looks like the wrong file type.
		   Doesn't need to return FAILURE status */
		*wrong_file_type = true;
		del_uchar(&buffer);
		free(absolute_path);
		fclose(fileptr);
		provide(status);
	}

	bool loop_was_interrupted = false;

	if(*offset == 0)
	{
		if(sha512_init(mdContext) == 1)
		{
			slog(ERROR,"SHA512 initialization failed\n");
			del_uchar(&buffer);
			free(absolute_path);
			fclose(fileptr);
			provide(FAILURE);
		}
	}

	if(config->dry_run == false)
	{
		while((len = fread(buffer->mem,sizeof(unsigned char),buffer_size,fileptr)) != 0)
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
				if(sha512_update(mdContext,buffer->mem,len) == 1)
				{
					slog(ERROR,"SHA512 update failed\n");
					status = FAILURE;
					break;
				}

				*offset += (sqlite3_int64)len;
			}
		}
	}

	del_uchar(&buffer);

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
