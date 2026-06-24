#include "sute.h"

enum
{
	TEST0023_EXPECTED_PATH_COUNT = 8
};

static const char *const test0023_expected_paths[TEST0023_EXPECTED_PATH_COUNT] = {
	".",
	"AAA",
	"AAA/BBB",
	"AAA/BBB/CCC",
	"AAA/BBB/CCC/a.txt",
	"AAA/BBB/uuu.txt",
	"AAA/tttt.txt",
	"sss.txt"
};

/**
 * @brief Compare two FTS entries by filename
 * @param first Pointer to first FTSENT structure
 * @param second Pointer to second FTSENT structure
 * @return Integer less than, equal to, or greater than zero when the first
 *         name is sorted before, matches, or is sorted after the second name
 */
static int test0023_compare_by_name(
#ifdef __CYGWIN__
	const FTSENT * const *first,
	const FTSENT * const *second)
#else
	const FTSENT **first,
	const FTSENT **second)
#endif
{
	return strcmp((*first)->fts_name,(*second)->fts_name);
}

/**
 * @brief Check path_build_relative() for one textual form of the same fixture root
 *
 * @details
 * The helper opens the supplied root path with FTS and verifies that every
 * directory or regular file in the fixture is represented by the same
 * root-relative path set. The root spelling itself may be plain relative,
 * dot-relative, contain `..`, or be absolute; the built child paths must stay
 * relative to that opened root
 *
 * @param[in] root_path_text Root path spelling passed to FTS
 * @return SUCCESS when all expected root-relative paths are produced
 */
static Return test0023_expect_paths_for_root(const char *root_path_text)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	bool seen[TEST0023_EXPECTED_PATH_COUNT] = {false};
	FTS *file_systems = NULL;

	m_create(char,root_path,MEMORY_STRING);
	m_create(char,relative_path,MEMORY_STRING);

	ASSERT(root_path_text != NULL);

	if(SUCCESS == status)
	{
		run(m_copy_string(root_path,root_path_text));
	}

	if(SUCCESS == status)
	{
		char *runtime_root_path = m_data(char,root_path);
		ASSERT(runtime_root_path != NULL);

		if(SUCCESS == status)
		{
			char *root_argv[] = {
				runtime_root_path,
				NULL
			};
			int fts_options = FTS_PHYSICAL;

#ifdef FTS_NOCHDIR
			fts_options |= FTS_NOCHDIR;
#endif

			file_systems = fts_open(root_argv,fts_options,test0023_compare_by_name);
			ASSERT(file_systems != NULL);
		}
	}

	if(SUCCESS == status)
	{
		FTSENT *entry = NULL;

		while((entry = fts_read(file_systems)) != NULL)
		{
			if(entry->fts_info != FTS_D && entry->fts_info != FTS_F)
			{
				continue;
			}

			run(path_build_relative(relative_path,entry));

			if(SUCCESS != status)
			{
				break;
			}

			const char *relative_path_text = m_text(relative_path);
			bool expected_path_seen = false;

			ASSERT(relative_path_text != NULL);
			ASSERT(relative_path_text[0] != '\0');
			ASSERT(relative_path_text[0] != '/');

			for(size_t i = 0; i < TEST0023_EXPECTED_PATH_COUNT; i++)
			{
				if(strcmp(relative_path_text,test0023_expected_paths[i]) == COMPLETED)
				{
					seen[i] = true;
					expected_path_seen = true;

					break;
				}
			}

			ASSERT(expected_path_seen == true);
		}
	}

	if(file_systems != NULL)
	{
		if(fts_close(file_systems) != COMPLETED)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		for(size_t i = 0; i < TEST0023_EXPECTED_PATH_COUNT; i++)
		{
			ASSERT(seen[i] == true);
		}
	}

	call(m_del(relative_path));
	call(m_del(root_path));

	provide(status);
}

/**
 * @brief Test path_build_relative() with several root path spellings
 *
 * @details
 * The test uses one real fixture directory and opens it through different
 * root spellings. Relative spellings are checked from the isolated test
 * workspace, because application tests also run against fixtures copied under
 * `$TMPDIR`. This checks the behavior that matters to the application: the
 * root path is chosen by the user and stored separately, while each file path
 * produced by traversal is built relative to that opened root
 *
 * @return Test status indicating success or failure
 */
Return test0023(void)
{
	INITTEST;

	bool cwd_changed = false;
	char *saved_cwd = NULL;
	const char *tmpdir = getenv("TMPDIR");
	const char *tmpdir_basename = NULL;
	const char *tmpdir_last_slash = NULL;
	const char *parent_basename = NULL;
	const char *parent_end = NULL;
	size_t parent_basename_length = 0U;

	m_create(char,parent_relative_root_path,MEMORY_STRING);
	m_create(char,grandparent_relative_root_path,MEMORY_STRING);
	m_create(char,absolute_root_path,MEMORY_STRING);

	/*
	 * Save the current directory because the relative-root checks below run
	 * from TMPDIR, just like application runs launched through testitall helpers
	 */
#if defined(__GLIBC__)
	saved_cwd = get_current_dir_name();
#else
	// Portable fallback for platforms without get_current_dir_name, such as macOS
	saved_cwd = getcwd(NULL,0);
#endif

	ASSERT(tmpdir != NULL);
	ASSERT(saved_cwd != NULL);

	/*
	 * Split TMPDIR into its final directory name and its parent directory name.
	 * Those pieces let the test spell the same fixture root through ../ and
	 * ../../ without hard-coding the generated temporary directory path
	 */
	if(SUCCESS == status)
	{
		tmpdir_last_slash = strrchr(tmpdir,'/');

		ASSERT(tmpdir_last_slash != NULL);
		ASSERT(tmpdir_last_slash[1] != '\0');
	}

	if(SUCCESS == status)
	{
		tmpdir_basename = tmpdir_last_slash + 1;
		parent_end = tmpdir_last_slash;

		while(parent_end > tmpdir && parent_end[-1] == '/')
		{
			parent_end--;
		}

		parent_basename = parent_end;

		while(parent_basename > tmpdir && parent_basename[-1] != '/')
		{
			parent_basename--;
		}

		parent_basename_length = (size_t)(parent_end - parent_basename);

		ASSERT(parent_basename_length > 0U);
		ASSERT(parent_basename_length <= INT_MAX);
	}

	/*
	 * Build two parent-relative root spellings that still resolve to the same
	 * fixture directory after the test changes into TMPDIR
	 */
	if(SUCCESS == status)
	{
		run(m_formatted_string(parent_relative_root_path,
			"../%s/tests/fixtures/4",
			tmpdir_basename));
		run(m_formatted_string(grandparent_relative_root_path,
			"../../%.*s/%s/tests/fixtures/4",
			(int)parent_basename_length,
			parent_basename,
			tmpdir_basename));
	}

	/*
	 * Run the relative spelling checks from TMPDIR. The main application tests
	 * also run commands from this isolated workspace, so this mirrors the
	 * command-line environment used by system-style tests
	 */
	if(SUCCESS == status)
	{
		ASSERT(chdir(tmpdir) == COMPLETED);

		if(SUCCESS == status)
		{
			cwd_changed = true;
		}
	}

	/*
	 * Open the same fixture root through several user-facing spellings and
	 * require path_build_relative() to produce the same root-relative path set
	 * for each spelling
	 */
	if(SUCCESS == status)
	{
		const char *const root_path_spellings[] = {
			"tests/fixtures/4",
			"tests/fixtures/4/",
			"./tests/fixtures/4",
			"tests/fixtures/diffs/../4",
			m_text(parent_relative_root_path),
			m_text(grandparent_relative_root_path),
			NULL
		};

		for(size_t i = 0; root_path_spellings[i] != NULL; i++)
		{
			run(test0023_expect_paths_for_root(root_path_spellings[i]));
		}
	}

	/*
	 * Restore the original working directory before checking the absolute root
	 * spelling. This keeps the rest of the test suite independent from this test
	 */
	if(cwd_changed == true && saved_cwd != NULL)
	{
		if(chdir(saved_cwd) != COMPLETED)
		{
			status = FAILURE;
		}

		cwd_changed = false;
	}

	/*
	 * Check the absolute spelling separately after restoring cwd. The expected
	 * root-relative paths must still be identical to the relative spelling cases
	 */
	if(SUCCESS == status)
	{
		run(construct_path("tests/fixtures/4",absolute_root_path));
		run(test0023_expect_paths_for_root(m_text(absolute_root_path)));
	}

	call(m_del(absolute_root_path));
	call(m_del(grandparent_relative_root_path));
	call(m_del(parent_relative_root_path));

	if(saved_cwd != NULL)
	{
		free(saved_cwd);
	}

	RETURN_STATUS;
}
