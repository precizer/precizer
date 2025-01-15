#include "testitall.h"

/*
 * Modification bits
 *
 */
typedef enum
{
    IDENTICAL                 = 0x00, // 0000
    NOT_EQUAL                 = 0x01, // 0001
    SIZE_CHANGED              = 0x02, // 010
    CREATION_TIME_CHANGED     = 0x04, // 0100
    MODIFICATION_TIME_CHANGED = 0x08  // 1000

} Changed;

/**
 * @brief Checks if the file's size, creation time, and modification time have
 *        not changed since the last crawl.
 *
 * Compares data from the FTS library file traversal with the stat
 * structure stored in SQLite from the previous probe.
 *
 * @param source		Source file stat structure
 * @param destination	Destination file stat structure
 *
 * @return Return status:
 *         - IDENTICAL: Files are identical
 *         - FAILURE: Error in comparison or invalid parameters
 *         - SIZE_CHANGED
 *         - MODIFICATION_TIME_CHANGED
 *         - CREATION_TIME_CHANGED
 */
static int compare_file_metadata_equivalence(
	const struct stat *source,
	const struct stat *destination
){
	/* Validate input parameters */
	if(NULL == source || NULL == destination)
	{
		return(FAILURE);
	}

	int result = IDENTICAL;

	/* Size of file, in bytes.  */
	if(source->st_size != destination->st_size)
	{
		result |= SIZE_CHANGED;

	}

	/* Modified timestamp */
	if(!(source->st_mtim.tv_sec == destination->st_mtim.tv_sec &&
	        source->st_mtim.tv_nsec == destination->st_mtim.tv_nsec))
	{
		result |= MODIFICATION_TIME_CHANGED;

	}

	/* Time of last status change  */
	if(!(source->st_ctim.tv_sec == destination->st_ctim.tv_sec &&
	        source->st_ctim.tv_nsec == destination->st_ctim.tv_nsec))
	{
		result |= CREATION_TIME_CHANGED;

	}

	return(result);
}

/**
 * @brief Prints all fields of the stat structure
 *
 * @param[in] st Pointer to the stat structure to print
 * @return Return Status of the operation
 */
Return print_stat(const struct stat *st)
{
	Return status = SUCCESS;
	char time_str[100];
	struct tm *tm_info;

	if(SUCCESS == status)
	{
		if(NULL == st)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		echo(STDERR,"----------------\n");
		echo(STDERR,"File information:\n");
		echo(STDERR,"Device ID: %lu\n",(unsigned long)st->st_dev);
		echo(STDERR,"Inode number: %lu\n",(unsigned long)st->st_ino);
		echo(STDERR,"Mode: %o (octal)\n",(unsigned int)st->st_mode);
		echo(STDERR,"Hard links: %lu\n",(unsigned long)st->st_nlink);
		echo(STDERR,"User ID: %u\n",st->st_uid);
		echo(STDERR,"Group ID: %u\n",st->st_gid);
		echo(STDERR,"Device ID (if special file): %lu\n",(unsigned long)st->st_rdev);
		echo(STDERR,"Total size: %ld bytes\n",(long)st->st_size);
		echo(STDERR,"Block size: %ld\n",(long)st->st_blksize);
		echo(STDERR,"Number of blocks: %ld\n",(long)st->st_blocks);

		// Access time
		tm_info = localtime(&st->st_atim.tv_sec);
		strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",tm_info);
		echo(STDERR,"Last access: %s.%09ld\n",time_str,st->st_atim.tv_nsec);

		// Modification time
		tm_info = localtime(&st->st_mtim.tv_sec);
		strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",tm_info);
		echo(STDERR,"Last modification: %s.%09ld\n",time_str,st->st_mtim.tv_nsec);

		// Status change time
		tm_info = localtime(&st->st_ctim.tv_sec);
		strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",tm_info);
		echo(STDERR,"Last status change: %s.%09ld\n",time_str,st->st_ctim.tv_nsec);

		// Print file type
		echo(STDERR,"File type: ");
		switch(st->st_mode & S_IFMT)
		{
			case S_IFBLK:
				echo(STDERR,"block device\n");
				break;
			case S_IFCHR:
				echo(STDERR,"character device\n");
				break;
			case S_IFDIR:
				echo(STDERR,"directory\n");
				break;
			case S_IFIFO:
				echo(STDERR,"FIFO/pipe\n");
				break;
			case S_IFLNK:
				echo(STDERR,"symlink\n");
				break;
			case S_IFREG:
				echo(STDERR,"regular file\n");
				break;
			case S_IFSOCK:
				echo(STDERR,"socket\n");
				break;
			default:
				echo(STDERR,"unknown\n");
				break;
		}

		// Print permissions
		echo(STDERR,"Permissions: ");
		echo(STDERR,(st->st_mode & S_IRUSR) ? "r" : "-");
		echo(STDERR,(st->st_mode & S_IWUSR) ? "w" : "-");
		echo(STDERR,(st->st_mode & S_IXUSR) ? "x" : "-");
		echo(STDERR,(st->st_mode & S_IRGRP) ? "r" : "-");
		echo(STDERR,(st->st_mode & S_IWGRP) ? "w" : "-");
		echo(STDERR,(st->st_mode & S_IXGRP) ? "x" : "-");
		echo(STDERR,(st->st_mode & S_IROTH) ? "r" : "-");
		echo(STDERR,(st->st_mode & S_IWOTH) ? "w" : "-");
		echo(STDERR,(st->st_mode & S_IXOTH) ? "x" : "-");
		echo(STDERR,"\n");
	}

	return(status);
}

/**
 *
 */
Return check_file_identity(
	const struct stat *stat1,
	const struct stat *stat2
){
	Return status = SUCCESS;

	if(IDENTICAL != compare_file_metadata_equivalence(stat1,stat2))
	{
		print_stat(stat1);
		print_stat(stat2);
		status = FAILURE;
	}

	return(status);
}
