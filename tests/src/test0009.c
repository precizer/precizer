#include "sute.h"

/**
 *
 * Validate file-level ignore filtering in a mixed directory
 *
 */
Return test0009_1(void)
{
	INITTEST;
	const char *db_filename = "database0009.db";

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009.db "
		"--ignore=\"^(?:skip_|tmp_).*\\.(?:log|bak)$\" "
		"tests/fixtures/ignore_include_cases/chaotic_filenames";

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

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	ASSERT(SUCCESS == delete_path(db_filename));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0009_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path(db_filename));

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
	const char *db_filename = "database0009_2.db";

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_2.db "
		"--ignore=\"^(?:skip_|tmp_|zeta_|omega_).+\" "
		"--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|zeta_z1-9vv\\.dat)$\" "
		"tests/fixtures/ignore_include_cases/chaotic_filenames";

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

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	ASSERT(SUCCESS == delete_path(db_filename));

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
	const char *db_filename = "database0009_3.db";

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_3.db "
		"--ignore=\"^chaotic_filenames(?:/|$)\" "
		"--include=\"^chaotic_filenames/(?:alpha_m0n9k2_zz\\.txt|hold_a1r9v-0pq\\.bak|keep_4xv7__m2\\.log|omega_77xy__aa\\.bin|xqwe_90210\\.md|zeta_z1-9vv\\.dat)$\" "
		"tests/fixtures/ignore_include_cases";

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

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	ASSERT(SUCCESS == delete_path(db_filename));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * @brief Validate update included branch in three passes
 */
static Return test0009_4(void)
{
	INITTEST;

	const char *db_filename = "database0009_4.db";
	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/ignore_include_cases/chaotic_filenames"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments_create = "--database=database0009_4.db "
		"--ignore=\"^(?:skip_|tmp_).+\" "
		"--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|tmp_z1-9vv\\.bak)$\" "
		"tests/fixtures/ignore_include_cases/chaotic_filenames";

	/*
	 * Create the baseline DB using the same ignore/include rules as the later update passes
	 * We intentionally build the initial record set as "tracked-after-filters" and not as "all files in directory"
	 * Update mode processes the current filtered set and does not retroactively delete rows that were inserted earlier
	 * If this first pass omitted filters, all 12 paths would be stored and the update-included scenario would validate a different logic branch
	 */
	ASSERT(SUCCESS == runit(arguments_create,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0009_004_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == add_string_to(" ","tests/fixtures/ignore_include_cases/chaotic_filenames/skip_4xv7__m2.log"));

	const char *arguments_update = "--update --database=database0009_4.db "
		"--ignore=\"^(?:skip_|tmp_).+\" "
		"--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|tmp_z1-9vv\\.bak)$\" "
		"tests/fixtures/ignore_include_cases/chaotic_filenames";

	// Update mode pass where only one included file has changed
	ASSERT(SUCCESS == runit(arguments_update,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0009_004_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == add_string_to(" ","tests/fixtures/ignore_include_cases/chaotic_filenames/tmp_qwe_90210.log"));
	ASSERT(SUCCESS == add_string_to(" ","tests/fixtures/ignore_include_cases/chaotic_filenames/tmp_z1-9vv.bak"));
	// Truncate a tracked non-included file to trigger the "update as empty" branch
	ASSERT(SUCCESS == truncate_file_to_zero_size("tests/fixtures/ignore_include_cases/chaotic_filenames/alpha_m0n9k2_zz.txt"));

	const char *arguments_update_watch = "--watch-timestamps --update --database=database0009_4.db "
		"--ignore=\"^(?:skip_|tmp_).+\" "
		"--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|tmp_z1-9vv\\.bak)$\" "
		"tests/fixtures/ignore_include_cases/chaotic_filenames";

	// Update mode with watch-timestamps enabled where one non-included file becomes empty and two included files are updated
	ASSERT(SUCCESS == runit(arguments_update_watch,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0009_004_3.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"omega_77xy__aa.bin",
		"skip_4xv7__m2.log",
		"tmp_qwe_90210.log",
		"tmp_z1-9vv.bak",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	ASSERT(SUCCESS == delete_path(db_filename));

	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/ignore_include_cases/chaotic_filenames"));

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
	TEST(test0009_4,"Create then update included files with and without detailed change output…");

	RETURN_STATUS;
}
