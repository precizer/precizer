#include "sute.h"

/**
 * @brief Check if file exists and get its stats
 *
 * @param path[in]      Path to file to check
 * @param stat_buf[out] Pointer to stat structure to be filled with file info
 *
 * @return Return status:
 *         - SUCCESS:  File exists and stat structure filled
 *         - FAILURE:  File not found or stat failed
 */
Return check_file(
	const char  *path,
	struct stat *stat_buf
){
	Return status = SUCCESS;

	if((NULL == path) || (NULL == stat_buf))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = stat(path,stat_buf);

		if(-1 == rc)
		{
			report("File not found: %s",path);
			status = FAILURE;
		} else if(rc < 0)
		{
			report("Stat of %s failed with error code: %d",path,rc);
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * @brief Prints all fields of the stat structure
 *
 * @param[in] st Pointer to the stat structure to print
 * @return Return Status of the operation
 */
Return print_stat(const struct stat *st){
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
		tm_info = localtime(&st->st_atime);
		strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",tm_info);
		echo(STDERR,"Last access: %s\n",time_str);

		// Modification time
		tm_info = localtime(&st->st_mtime);

		strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",tm_info);
		echo(STDERR,"Last modification: %s\n",time_str);

		// Status change time
		tm_info = localtime(&st->st_ctime);

		strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",tm_info);
		echo(STDERR,"Last status change: %s\n",time_str);

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
 * @brief Constructs full file path by combining TMPDIR environment variable with filename
 *
 * @param[in] filename Name of the file to append to TMPDIR path
 * @param[out] full_path Pointer to string pointer that will store the allocated path
 * @return Return Status of the operation
 */
Return construct_path(
	const char *filename,
	char       **full_path
){
	Return status = SUCCESS;
	const char *tmp_dir = NULL;
	size_t path_len = 0;

	if(SUCCESS == status)
	{
		if(NULL == filename || NULL == full_path)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		tmp_dir = getenv("TMPDIR");

		if(NULL == tmp_dir)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		path_len = strlen(tmp_dir) + strlen(filename) + 2;   // +2 for '/' and '\0'
		*full_path = (char *)malloc(path_len);

		if(NULL == *full_path)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int print_result = snprintf(*full_path,path_len,"%s/%s",tmp_dir,filename);

		if(print_result < 0 || (size_t)print_result >= path_len)
		{
			free(*full_path);
			*full_path = NULL;
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * The db file should not be created in the Dry Run mode
 */
static Return dry_run_mode_1_test(void){
	Return status = SUCCESS;

	const char *command = "export TESTING=true;cd ${TMPDIR};" \
	        "./precizer --dry-run --database=database1.db tests/examples/diffs/diff1";

	MSTRUCT(mem_char,result);

	ASSERT(SUCCESS == execute_command(command,result,0));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	del_char(&result);

	// Does file exists or not
	const char *filename = "database1.db";
	char *path = NULL;
	bool file_exists = false;

	ASSERT(SUCCESS == construct_path(filename,&path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,path));

	free(path);

	// Should not be exists
	ASSERT(file_exists == false);

	RETURN_STATUS;
}

/**
 * The db file should not be updated in the Dry Run mode
 */
static Return dry_run_mode_2_test(void){
	Return status = SUCCESS;
	// Create memory for the result
	MSTRUCT(mem_char,result);
	struct stat stat1;
	struct stat stat2;
	char *path = NULL;
	char *pattern = NULL;
	const char *filename = "database1.db";
	const char *command = "export TESTING=true;cd ${TMPDIR};" \
	        "./precizer --database=database1.db tests/examples/diffs/diff1";

	// Preparation for tests
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
		"cp -r tests/examples/ tests/examples_backup;",0));

	ASSERT(SUCCESS == construct_path(filename,&path));

	ASSERT(SUCCESS == external_call(command,0));

	#if 0
	echo(STDOUT,"Path: %s\n",path);
	#endif

	ASSERT(SUCCESS == check_file(path,&stat1));

	command = "export TESTING=true;cd ${TMPDIR};" \
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" \
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" \
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;" \
	        "./precizer --dry-run --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,0));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	del_char(&result);

	ASSERT(SUCCESS == check_file(path,&stat2));

	if(SUCCESS == status)
	{
		if(memcmp(&stat1,&stat2,sizeof(struct stat)) != 0)
		{
			print_stat(&stat1);
			print_stat(&stat2);
			status = FAILURE;
		}
	}

	// Compare against the sample. A message should be displayed indicating
	// that the --db-clean-ignored option must be specified for permanent
	// removal of ignored files from the database
	command = "export TESTING=true;cd ${TMPDIR};" \
	        "./precizer --dry-run --ignore=\"tests/examples/diffs/diff1/1/AAA/ZAW/*\"" \
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,0));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	ASSERT(SUCCESS == get_file_content("templates/0013_002_1.txt",&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	ASSERT(SUCCESS == check_file(path,&stat2));

	if(SUCCESS == status)
	{
		if(memcmp(&stat1,&stat2,sizeof(struct stat)) != 0)
		{
			print_stat(&stat1);
			print_stat(&stat2);
			status = FAILURE;
		}
	}

	// Dry Run mode permanent deletion of all ignored file
	// references from the database
	command = "export TESTING=true;cd ${TMPDIR};" \
	        "./precizer --db-clean-ignored --ignore=\"tests/examples/diffs/diff1/1/AAA/ZAW/*\"" \
	        " --update --dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,0));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	ASSERT(SUCCESS == get_file_content("templates/0013_002_3.txt",&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	ASSERT(SUCCESS == check_file(path,&stat2));

	if(SUCCESS == status)
	{
		if(memcmp(&stat1,&stat2,sizeof(struct stat)) != 0)
		{
			print_stat(&stat1);
			print_stat(&stat2);
			status = FAILURE;
		}
	}

	command = "export TESTING=true;cd ${TMPDIR};" \
	        "./precizer --db-clean-ignored --ignore=\"path2/AAA/ZAW/*\"" \
	        " --update --dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,0));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	ASSERT(SUCCESS == get_file_content("templates/0013_002_4.txt",&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	ASSERT(SUCCESS == check_file(path,&stat2));

	if(SUCCESS == status)
	{
		if(memcmp(&stat1,&stat2,sizeof(struct stat)) != 0)
		{
			print_stat(&stat1);
			print_stat(&stat2);
			status = FAILURE;
		}
	}

	free(path);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
		"rm database1.db;" \
		"rm -rf tests/examples/;" \
		"mv tests/examples_backup/ tests/examples/",0));

	RETURN_STATUS;
}

// Main test runner
Return test0013(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(dry_run_mode_1_test,"The DB file should not be created…");
	TEST(dry_run_mode_2_test,"The DB file should not be updated…");

	RETURN_STATUS;
}
