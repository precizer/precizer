#include "sute.h"

#define TARGET_FILE "${TMPDIR}/tests/examples/diffs/diff1/path1/AAA/BCB/CCC/a.txt"
#define LOCKED_TAMPER_PATH "path1/AAA/ZAW/A/b/c/a_file.txt"
#define LOCKED_TAMPER_FILE "tests/examples/diffs/diff1/" LOCKED_TAMPER_PATH

static Return tamper_locked_checksum(
	const char *db_filename,
	const char *relative_path)
{
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "UPDATE files SET sha512 = (substr(sha512,1,2) || X'BEEF' || substr(sha512,5)) "
	                  "WHERE relative_path = ?1;";
	create(char,db_path);

	if(SUCCESS == status && (db_filename == NULL || relative_path == NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(getcstring(db_path),&db,SQLITE_OPEN_READWRITE,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_DONE != sqlite3_step(stmt))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && sqlite3_changes(db) < 1)
	{
		status = FAILURE;
	}

	if(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	if(db != NULL)
	{
		(void)sqlite3_close(db);
	}

	del(db_path);

	return(status);
}

static Return tamper_locked_file_bytes(
	const char *relative_path)
{
	Return status = SUCCESS;
	int fd = -1;
	struct stat before = {0};
	unsigned char buffer[2] = {0};
	struct timespec times[2] = {{0}};
	create(char,file_path);

	if(SUCCESS == status && relative_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path,file_path);
	}

	if(SUCCESS == status && (fd = open(getcstring(file_path),O_RDWR)) < 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && fstat(fd,&before) != 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && before.st_size < (off_t)sizeof(buffer))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && pread(fd,buffer,sizeof(buffer),0) != (ssize_t)sizeof(buffer))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		buffer[0] = (unsigned char)~buffer[0];
		buffer[1] = (unsigned char)~buffer[1];
	}

	if(SUCCESS == status && pwrite(fd,buffer,sizeof(buffer),0) != (ssize_t)sizeof(buffer))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		// Best effort: restore atime/mtime; ctime will still update on POSIX.
		times[0] = before.st_atim;
		times[1] = before.st_mtim;

		if(futimens(fd,times) != 0)
		{
			status = FAILURE;
		}
	}

	if(fd >= 0)
	{
		(void)close(fd);
	}

	del(file_path);

	return(status);
}

/**
 * Size change with locked checksums should raise a warning during update.
 */
static Return test0030_1_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s1.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("printf 'pad' >> " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s1.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s1.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Timestamp drift with --watch-timestamps produces a warning for locked entries.
 */
static Return test0030_2_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s2.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s2.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update --watch-timestamps " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s2.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s2.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Timestamp drift without --watch-timestamps should complete successfully.
 */
static Return test0030_3_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s3.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s3.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s3.db tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	filename = "templates/0030_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s3.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Rehashing locked files while watching timestamps should still succeed.
 */
static Return test0030_4_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s4.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s4.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update --watch-timestamps --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s4.db tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	filename = "templates/0030_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s4.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Tampering with a locked checksum in the DB should trigger a warning during rehash.
 */
static Return test0030_5_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s5.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s5.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_005_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	// Corrupt the stored checksum for a locked file without touching it on disk.
	ASSERT(SUCCESS == tamper_locked_checksum("lock_s5.db",LOCKED_TAMPER_PATH));

	ASSERT(SUCCESS == runit("--update --watch-timestamps --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s5.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_005_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s5.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Locked content change without DB tampering should trigger a warning during rehash.
 */
static Return test0030_6_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s6.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s6.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_006_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == tamper_locked_file_bytes(LOCKED_TAMPER_FILE));

	ASSERT(SUCCESS == runit("--update --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s6.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_006_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s6.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Locked content change with --watch-timestamps should trigger a warning during rehash.
 */
static Return test0030_7_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s7.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s7.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_007_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == tamper_locked_file_bytes(LOCKED_TAMPER_FILE));

	ASSERT(SUCCESS == runit("--update --watch-timestamps --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s7.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_007_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s7.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Example 10: scenarios with --lock-checksum, --rehash-locked, and --watch-timestamps
 *
 */
Return test0030(void)
{
	INITTEST;

	TEST(test0030_1_test,"Size change with locked checksum triggers a warning…");
	TEST(test0030_2_test,"Timestamp drift with --watch-timestamps triggers a warning…");
	TEST(test0030_3_test,"Timestamp drift without --watch-timestamps completes successfully…");
	TEST(test0030_4_test,"Timestamp drift with --watch-timestamps and --rehash-locked completes successfully…");
	TEST(test0030_5_test,"Locked checksum mismatch in DB triggers a warning…");
	TEST(test0030_6_test,"Locked file content change triggers a warning…");
//	TEST(test0030_7_test,"Locked file content change with --watch-timestamps triggers a warning…");

	RETURN_STATUS;
}
