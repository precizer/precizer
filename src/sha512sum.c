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
static long нow_much_memory_can_be_allocated_for_the_buffer(void)
{
	// Default value is 1MB buffer. Is it too big for embedded and IoT?
	const long buffer_size = 1024*1024;

	long pages = sysconf(_SC_AVPHYS_PAGES); // Number of actually free pages
	long page_size = sysconf(_SC_PAGESIZE); // Page size in bytes

	if(pages == -1 || page_size == -1) {
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
	const char               *relative_path,
	const short unsigned int *relative_path_size,
	unsigned char            *sha512,
	sqlite3_int64            *offset,
	SHA512_Context           *mdContext
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	const long buffer_size = нow_much_memory_can_be_allocated_for_the_buffer();
	unsigned char *buffer = (unsigned char *)calloc((size_t)buffer_size, sizeof(unsigned char));
	FILE *fileptr = NULL;
	size_t len = 0;

	fileptr = fopen(relative_path,"rb");

	if(fileptr == NULL)
	{
		// The variable in the stack is extremely fast
		char absolute_path[config->running_dir_size + *relative_path_size + 1];

		strcpy(absolute_path,config->running_dir);
		absolute_path[strlen(config->running_dir)] = '/';
		absolute_path[strlen(config->running_dir) + 1] = '\0';
		strcat(absolute_path,relative_path);

		fileptr = fopen(absolute_path,"rb");

		if(fileptr == NULL)
		{
			slog(ERROR,"Can open the file using neither relative %s nor absolute %s path\n",relative_path,absolute_path);
			status = FAILURE;
			return(status);
		}
	}

	// It moves the file pointer "offset" bytes from the beginning of the file
	fseek(fileptr,*offset,SEEK_SET);

	bool loop_was_interrupted = false;

	if(*offset == 0)
	{
		sha512_init(mdContext);
	}

	while((len = fread(buffer,1,(size_t)buffer_size,fileptr)) != 0)      // read from infile
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true)
		{
			loop_was_interrupted = true;
			break;
		}
		sha512_update(mdContext,buffer,len);
		*offset += (sqlite3_int64)len;
	}

	free(buffer);

	fclose(fileptr); // Close the file

	if(loop_was_interrupted == false)
	{
		*offset = 0;
		sha512_final(mdContext,sha512);
	}

#if 0
	for(size_t i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		printf("%02x",sha512[i]);
	}
	putchar('\n');
#endif

	return(status);
}
