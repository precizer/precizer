#include "sute.h"

static Return assert_db_paths_match(
	const char        *db_filename,
	const char *const *expected_paths,
	const int         expected_count)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT relative_path FROM files ORDER BY relative_path ASC;";
	create(char,db_path);

	if(SUCCESS == status && (db_filename == NULL || expected_paths == NULL || expected_count < 0))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(getcstring(db_path),&db,SQLITE_OPEN_READONLY,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	int index = 0;

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		while(rc == SQLITE_ROW)
		{
			if(index >= expected_count)
			{
				status = FAILURE;
				break;
			}

			const unsigned char *db_path_text = sqlite3_column_text(stmt,0);

			if(db_path_text == NULL || strcmp((const char *)db_path_text,expected_paths[index]) != 0)
			{
				status = FAILURE;
				break;
			}

			index++;
			rc = sqlite3_step(stmt);
		}

		if(SUCCESS == status && rc != SQLITE_DONE)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && index != expected_count)
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

/**
 *
 * Validate file-level ignore filtering in a mixed directory
 *
 */
Return test0009_1(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009.db "
		"--ignore=\"^(?:skip_|tmp_).*\\.(?:log|bak)$\" "
		"tests/examples/ignore_include_cases/chaotic_filenames";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0009_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"omega_77xy__aa.bin",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == assert_db_paths_match("database0009.db",expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	const char *command = "rm \"${TMPDIR}/database0009.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0009_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Validate include over ignore with chaotic filenames
 *
 */
static Return test0009_2(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_2.db "
		"--ignore=\"^(?:skip_|tmp_|zeta_|omega_).+\" "
		"--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|zeta_z1-9vv\\.dat)$\" "
		"tests/examples/ignore_include_cases/chaotic_filenames";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0009_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"skip_4xv7__m2.log",
		"tmp_qwe_90210.log",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == assert_db_paths_match("database0009_2.db",expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	const char *command = "rm \"${TMPDIR}/database0009_2.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Validate whole-directory ignore with selective include
 *
 */
static Return test0009_3(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_3.db "
		"--ignore=\"^chaotic_filenames(?:/|$)\" "
		"--include=\"^chaotic_filenames/(?:alpha_m0n9k2_zz\\.txt|hold_a1r9v-0pq\\.bak|keep_4xv7__m2\\.log|omega_77xy__aa\\.bin|xqwe_90210\\.md|zeta_z1-9vv\\.dat)$\" "
		"tests/examples/ignore_include_cases";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0009_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	const char *expected_paths[] =
	{
		"chaotic_filenames/alpha_m0n9k2_zz.txt",
		"chaotic_filenames/hold_a1r9v-0pq.bak",
		"chaotic_filenames/keep_4xv7__m2.log",
		"chaotic_filenames/omega_77xy__aa.bin",
		"chaotic_filenames/xqwe_90210.md",
		"chaotic_filenames/zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == assert_db_paths_match("database0009_3.db",expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	const char *command = "rm \"${TMPDIR}/database0009_3.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

Return test0009(void)
{
	INITTEST;

	TEST(test0009_1,"Ignore regexp splits chaotic filenames into tracked and skipped sets…");
	TEST(test0009_2,"Ignore most files and include back selected ones…");
	TEST(test0009_3,"Directory ignore with selective child include…");

	RETURN_STATUS;
}
