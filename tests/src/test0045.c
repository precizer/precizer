#include "sute.h"
#include <sysexits.h>

typedef struct
{
	const char *path;
	bool required;
	bool has_entries;
	Return result;
} ReaderCapture;

static ReaderCapture *active_reader = NULL;

/**
 * @brief Preserve the reader's return value while capturing its diagnostics
 */
static void capture_reader(void)
{
	char program_name[] = "testitall";
	struct argp_state state = {.name = program_name,.err_stream = stderr,.flags = ARGP_NO_EXIT};
	active_reader->result = read_config_file(active_reader->path,
		active_reader->required,&active_reader->has_entries,&state);
}

/**
 * @brief Apply a file to fresh defaults and check the complete diagnostic output
 * @param relative_path Configuration path under TMPDIR
 * @param required Whether absence must cause an error
 * @param expected_result Expected reader return status
 * @param expected_entries Whether a valid record must have been encountered
 * @param error_template Complete stderr template, or NULL for silence
 * @return SUCCESS when the reader and all captured output match
 */
static Return check_reader(
	const char *relative_path,
	bool       required,
	Return     expected_result,
	bool       expected_entries,
	const char *error_template)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	/* Cleanup must discard a pending diagnostic line before the next run */
	config->config_file_error_line = 12;
	free_config();
	ASSERT(config->config_file_error_line == -1);
	ASSERT(SUCCESS == init_config_and_check_output());
	ASSERT(config->silent == false);
	ASSERT(config->verbose == false);
	ASSERT(config->output == OUTPUT_FULL);
	ASSERT(config->config_file_error_line == -1);
	m_create(char,path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	ReaderCapture capture = {
		.required = required,
		.has_entries = true,
		.result = FAILURE
	};

	ASSERT(SUCCESS == construct_path(relative_path,path));
	IF(SUCCESS == status)
	{
		capture.path = m_text(path);
		active_reader = &capture;
	}
	ASSERT(SUCCESS == function_capture(capture_reader,captured_stdout,captured_stderr));
	active_reader = NULL;
	ASSERT(capture.result == expected_result);
	ASSERT(capture.has_entries == expected_entries);
	ASSERT(config->config_file_error_line == -1);
	if(expected_result & FAILURE)
	{
		ASSERT(SUCCESS == get_file_content("templates/0045_034.txt",pattern));
		ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,"templates/0045_034.txt"));
	} else {
		ASSERT(captured_stdout->length == 0U);
	}

	if(error_template != NULL)
	{
		ASSERT(SUCCESS == get_file_content(error_template,pattern));
		ASSERT(SUCCESS == match_pattern(captured_stderr,pattern,error_template));
	} else {
		ASSERT(captured_stderr->length == 0U);
	}

	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(path));
	deliver(status);
}

/**
 * @brief Apply repeated settings across sections and retain their strings after reading finishes
 * @return Return status for scalar, list, alias, and output-setting application
 */
static Return test0045_1(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001.conf",true,SUCCESS,true,NULL));
	ASSERT(config->roots.length == 4U);
	memory *roots = m_data(memory,conf(roots));
	ASSERT(strcmp(m_text(&roots[0]),"/data/first") == 0);
	ASSERT(strcmp(m_text(&roots[1]),"/data/second#suffix") == 0);
	ASSERT(strcmp(m_text(&roots[2]),"root with spaces") == 0);
	ASSERT(strcmp(m_text(&roots[3]),"[literal continuation]") == 0);

	const char *expected_ignore[] = {
		"^cache/","^tmp/","^something_eles/","^other/","^things/","^before#after"
	};
	ASSERT(config->ignore != NULL);

	for(size_t i = 0; i < sizeof(expected_ignore) / sizeof(expected_ignore[0]) && SUCCESS == status; ++i)
	{
		ASSERT(config->ignore[i] != NULL);
		ASSERT(strcmp(config->ignore[i],expected_ignore[i]) == 0);
	}
	ASSERT(config->ignore[6] == NULL);
	ASSERT(config->include != NULL);
	ASSERT(config->include[0] != NULL && config->include[1] != NULL);
	ASSERT(strcmp(config->include[0],"^one;two=three:four/") == 0);
	ASSERT(strcmp(config->include[1],"^following/") == 0);
	ASSERT(config->include[2] == NULL);
	ASSERT(config->include_specified == true);
	ASSERT(config->db_drop_ignored == false);
	ASSERT(config->db_drop_inaccessible == false);
	ASSERT(config->one_file_system == false);
	ASSERT(config->db_check_level == QUICK);
	ASSERT(config->maxdepth == 7);
	ASSERT(strcmp(confstr(db_primary_file_path),"/second/final.db") == 0);
	ASSERT(strcmp(confstr(db_file_name),"final.db") == 0);
	ASSERT(config->compare_filter == (CF_CHECKSUM_MISMATCH | CF_FIRST_SOURCE));
	ASSERT(config->dry_run == true && config->dry_run_with_checksums == true);
	ASSERT(config->verbose == false);
	ASSERT(config->silent == true);
	ASSERT(rational_logger_mode == SILENT);
	ASSERT(rational_color_mode == COLOR_MODE_NEVER);

	/* Mixed-case FULL replaces QUICK instead of retaining the earlier value */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001_5.conf",true,SUCCESS,true,NULL));
	ASSERT(config->db_check_level == FULL);

	/* The largest supported traversal depth is accepted without truncation */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001_6.conf",true,SUCCESS,true,NULL));
	ASSERT(config->maxdepth == 32767);

	/* BOM, CRLF, initial indentation, and an unterminated continuation */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001_1.conf",true,SUCCESS,true,NULL));
	ASSERT(config->roots.length == 2U);
	roots = m_data(memory,conf(roots));
	ASSERT(strcmp(m_text(&roots[0]),"first") == 0);
	ASSERT(strcmp(m_text(&roots[1]),"second") == 0);

	/* A selected file can contain no records at all */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001_2.conf",true,SUCCESS,false,NULL));
	ASSERT(config->roots.length == 0U);
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001_3.conf",true,SUCCESS,false,NULL));
	ASSERT(config->roots.length == 0U);

	/* Section headers alone do not count as settings */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_001_4.conf",true,SUCCESS,false,NULL));
	ASSERT(config->roots.length == 0U);

	RETURN_STATUS;
}

/**
 * @brief Reject invalid syntax and keys before consuming any later records
 * @return Return status for first-error handling and original line numbers
 */
static Return test0045_2(void)
{
	INITTEST;
	static const struct {
		const char *config_template;
		const char *error_template;
	} cases[] = {
		{"tests/templates/0045_002_1.conf","templates/0045_002.txt"},
		{"tests/templates/0045_002_2.conf","templates/0045_002_2.txt"},
		{"tests/templates/0045_002_3.conf","templates/0045_002_2.txt"},
		{"tests/templates/0045_002_4.conf","templates/0045_002_2.txt"},
		{"tests/templates/0045_002_5.conf","templates/0045_002_5.txt"},
		{"tests/templates/0045_002_6.conf","templates/0045_002_6.txt"},
		{"tests/templates/0045_002_7.conf","templates/0045_002_7.txt"},
		{"tests/templates/0045_002_8.conf","templates/0045_002_8.txt"},
		{"tests/templates/0045_002_9.conf","templates/0045_002_9.txt"},
		{"tests/templates/0045_002_10.conf","templates/0045_002_10.txt"},
		{"tests/templates/0045_002_11.conf","templates/0045_002_11.txt"}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,FAILURE,true,
			cases[i].error_template));
		ASSERT(config->roots.length == 1U);
	}

	/* A first-line failure must prevent the later valid key from setting the flag */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_002_12.conf",true,FAILURE,false,
		"templates/0045_002_12.txt"));
	ASSERT(config->roots.length == 0U);

	RETURN_STATUS;
}

/**
 * @brief Reject an oversized line and stop before applying subsequent settings
 * @return Return status for the parser's long-line error and unread later settings
 */
static Return test0045_3(void)
{
	INITTEST;
	/* inih reports the error after recognizing the key from the buffered prefix */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_003.conf",true,FAILURE,true,
		"templates/0045_003.txt"));
	/* The valid path after the oversized line must not be applied */
	ASSERT(config->roots.length == 0U);

	RETURN_STATUS;
}

/**
 * @brief Distinguish optional absence from required-file and read failures
 * @return Return status for filesystem failure handling
 */
static Return test0045_4(void)
{
	INITTEST;

	ASSERT(SUCCESS == delete_path_if_present("0045_reader.conf"));
	ASSERT(SUCCESS == check_reader("0045_reader.conf",false,SUCCESS,false,NULL));
	ASSERT(SUCCESS == check_reader("0045_reader.conf",true,FAILURE,false,
		"templates/0045_004_1.txt"));

	/* A regular file used as a parent produces ENOTDIR even with elevated privileges */
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0045_001_2.conf",
		"0045_reader.conf",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == check_reader("0045_reader.conf/child",false,FAILURE,false,
		"templates/0045_004_2.txt"));
	ASSERT(SUCCESS == check_reader("0045_reader.conf/child",true,FAILURE,false,
		"templates/0045_004_2.txt"));

	/* A directory opens successfully but causes a read error on the first read */
	ASSERT(SUCCESS == create_directory("0045_config_directory"));
	ASSERT(SUCCESS == check_reader("0045_config_directory",true,FAILURE,false,
		"templates/0045_004_3.txt"));
	call(delete_path_if_present("0045_config_directory"));

	call(delete_path_if_present("0045_reader.conf"));
	RETURN_STATUS;
}

/**
 * @brief Apply the last boolean value and update the flag's related settings
 * @return Return status for boolean spellings, repeated flags, and dry-run modes
 */
static Return test0045_5(void)
{
	INITTEST;
	free_config();
	ASSERT(SUCCESS == init_config_and_check_output());
	const LOGMODES default_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const struct {
		const char *config_template;
		bool enabled;
	} cases[] = {
		{"tests/templates/0045_005_1.conf",true},
		{"tests/templates/0045_005_2.conf",false},
		{"tests/templates/0045_005_3.conf",false},
		{"tests/templates/0045_005_4.conf",true},
		{"tests/templates/0045_005_5.conf",false},
		{"tests/templates/0045_005_6.conf",false},
		{"tests/templates/0045_005_7.conf",true},
		{"tests/templates/0045_005_8.conf",false},
		{"tests/templates/0045_005_9.conf",false},
		{"tests/templates/0045_005_10.conf",true},
		{"tests/templates/0045_005_11.conf",false},
		{"tests/templates/0045_005_12.conf",false},
		{"tests/templates/0045_005_13.conf",true},
		{"tests/templates/0045_005_14.conf",false},
		{"tests/templates/0045_005_15.conf",false},
		{"tests/templates/0045_005_16.conf",true}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,SUCCESS,true,NULL));
		ASSERT(config->progress == cases[i].enabled);
		ASSERT(config->update == cases[i].enabled);
		ASSERT(config->dry_run == cases[i].enabled);
		ASSERT(config->rehash_locked == cases[i].enabled);
		ASSERT(config->one_file_system == cases[i].enabled);
		ASSERT(config->db_drop_ignored == cases[i].enabled);
		ASSERT(config->db_drop_inaccessible == cases[i].enabled);
		ASSERT(config->watch_timestamps == cases[i].enabled);
		ASSERT(config->force == cases[i].enabled);
		ASSERT(config->quiet_ignored == cases[i].enabled);
		ASSERT(config->silent == cases[i].enabled);
		ASSERT(config->verbose == cases[i].enabled);
		ASSERT(config->show_remembered_messages_at_exit == cases[i].enabled);
		ASSERT(config->dry_run_with_checksums == false);

		if(cases[i].enabled == true)
		{
			ASSERT(rational_logger_mode == SILENT);
		} else {
			ASSERT(rational_logger_mode == default_logger_mode);
		}
	}

	/* A plain true value replaces the checksum mode; a later named mode enables it */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_005_17.conf",true,SUCCESS,true,NULL));
	ASSERT(config->dry_run == true && config->dry_run_with_checksums == false);
	ASSERT(SUCCESS == check_reader("tests/templates/0045_005_18.conf",true,SUCCESS,true,NULL));
	ASSERT(config->dry_run == true && config->dry_run_with_checksums == true);

	/* Disabling an output mode preserves a different active mode */
	const struct {
		const char *config_template;
		LOGMODES logger_mode;
		bool verbose;
	} output_cases[] = {
		{"tests/templates/0045_005_19.conf",default_logger_mode,false},
		{"tests/templates/0045_005_20.conf",default_logger_mode,false},
		{"tests/templates/0045_005_21.conf",SILENT,false},
		{"tests/templates/0045_005_22.conf",VERBOSE,true}
	};

	for(size_t i = 0; i < sizeof(output_cases) / sizeof(output_cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(output_cases[i].config_template,true,SUCCESS,true,NULL));
		ASSERT(rational_logger_mode == output_cases[i].logger_mode);
		ASSERT(config->verbose == output_cases[i].verbose);
	}

	RETURN_STATUS;
}

/**
 * @brief Attribute shared option errors to their original configuration lines
 * @return Return status for invalid values and immediate parsing termination
 */
static Return test0045_6(void)
{
	INITTEST;
	static const struct {
		const char *config_template;
		const char *error_template;
	} cases[] = {
		{"tests/templates/0045_006_1.conf","templates/0045_006_1.txt"},
		{"tests/templates/0045_006_2.conf","templates/0045_006_2.txt"},
		{"tests/templates/0045_006_3.conf","templates/0045_006_3.txt"},
		{"tests/templates/0045_006_4.conf","templates/0045_006_4.txt"},
		{"tests/templates/0045_006_5.conf","templates/0045_006_5.txt"},
		{"tests/templates/0045_006_6.conf","templates/0045_006_6.txt"},
		{"tests/templates/0045_006_7.conf","templates/0045_006_7.txt"},
		{"tests/templates/0045_006_8.conf","templates/0045_006_8.txt"},
		{"tests/templates/0045_006_9.conf","templates/0045_015.txt"},
		{"tests/templates/0045_006_10.conf","templates/0045_006_10.txt"},
		{"tests/templates/0045_006_11.conf","templates/0045_006_11.txt"},
		{"tests/templates/0045_006_12.conf","templates/0045_006_12.txt"},
		{"tests/templates/0045_006_13.conf","templates/0045_006_13.txt"},
		{"tests/templates/0045_006_14.conf","templates/0045_006_14.txt"},
		{"tests/templates/0045_006_15.conf","templates/0045_015.txt"},
		{"tests/templates/0045_006_16.conf","templates/0045_015.txt"},
		{"tests/templates/0045_006_17.conf","templates/0045_006_12.txt"},
		{"tests/templates/0045_006_18.conf","templates/0045_006_12.txt"}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,FAILURE,true,cases[i].error_template));
		ASSERT(config->roots.length == 1U);
		memory *roots = m_data(memory,conf(roots));
		ASSERT(strcmp(m_text(&roots[0]),"first") == 0);
	}

	RETURN_STATUS;
}

/**
 * @brief Retain regex text from repeated keys and continuations for working compilation
 * @return Return status for valid and empty expressions in all three regex lists
 */
static Return test0045_7(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_reader("tests/templates/0045_007.conf",true,SUCCESS,true,NULL));

	char **patterns[] = {config->ignore,config->include,config->lock_checksum};

	for(size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]) && SUCCESS == status; ++i)
	{
		ASSERT(patterns[i] != NULL);
		ASSERT(patterns[i][0] != NULL && patterns[i][1] != NULL && patterns[i][2] != NULL);
		ASSERT(strcmp(patterns[i][0],"^first/") == 0);
		ASSERT(strcmp(patterns[i][1],"^second/") == 0);
		ASSERT(strcmp(patterns[i][2],"") == 0);
		ASSERT(patterns[i][3] == NULL);
	}

	/* Reading keeps no compiled objects; working compilation creates them */
	ASSERT(config->ignore_pcre_compiled == NULL);
	ASSERT(config->include_pcre_compiled == NULL);
	ASSERT(config->lock_checksum_pcre_compiled == NULL);
	ASSERT(SUCCESS == compile_patterns());
	pcre2_code **compiled[] = {
		config->ignore_pcre_compiled,config->include_pcre_compiled,config->lock_checksum_pcre_compiled
	};

	for(size_t i = 0; i < sizeof(compiled) / sizeof(compiled[0]) && SUCCESS == status; ++i)
	{
		ASSERT(compiled[i] != NULL);
		ASSERT(compiled[i][0] != NULL && compiled[i][1] != NULL && compiled[i][2] != NULL);
		ASSERT(compiled[i][3] == NULL);
	}

	RETURN_STATUS;
}

/**
 * @brief Check the compiler's failure status while its normal output is captured
 * @return SUCCESS when working compilation rejects the stored expression
 */
static Return capture_pattern_compilation_failure(void)
{
	INITTEST;
	ASSERT(FAILURE == compile_patterns());
	deliver(status);
}

/**
 * @brief Defer regex errors until working compilation and prioritize configuration parsing errors
 * @return Return status for first entries, repeated keys, and continuation values
 */
static Return test0045_8(void)
{
	INITTEST;
	m_create(char,pattern,MEMORY_STRING);
	ASSERT(SUCCESS == get_file_content("templates/0045_008_1.txt",pattern));
	const struct {
		const char *config_template;
		/* Address of the Config list populated after check_reader() resets defaults */
		char ***patterns;
		bool has_valid_pattern;
	} cases[] = {
		{"tests/templates/0045_008_1.conf",&config->ignore,false},
		{"tests/templates/0045_008_2.conf",&config->include,false},
		{"tests/templates/0045_008_3.conf",&config->lock_checksum,false},
		{"tests/templates/0045_008_4.conf",&config->ignore,true},
		{"tests/templates/0045_008_5.conf",&config->include,true},
		{"tests/templates/0045_008_6.conf",&config->lock_checksum,true},
		{"tests/templates/0045_008_7.conf",&config->ignore,true},
		{"tests/templates/0045_008_8.conf",&config->include,true},
		{"tests/templates/0045_008_9.conf",&config->lock_checksum,true}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,SUCCESS,true,NULL));
		ASSERT(config->roots.length == 1U);
		memory *roots = m_data(memory,conf(roots));
		ASSERT(strcmp(m_text(&roots[0]),"after-pattern") == 0);
		ASSERT(*cases[i].patterns != NULL);
		ASSERT((*cases[i].patterns)[0] != NULL);

		if(cases[i].has_valid_pattern == true)
		{
			ASSERT(strcmp((*cases[i].patterns)[0],"^first/") == 0);
			ASSERT((*cases[i].patterns)[1] != NULL);
			ASSERT(strcmp((*cases[i].patterns)[1],"(") == 0);
			ASSERT((*cases[i].patterns)[2] == NULL);
		} else {
			ASSERT(strcmp((*cases[i].patterns)[0],"(") == 0);
			ASSERT((*cases[i].patterns)[1] == NULL);
		}

		/* Use the normal uncolored diagnostic, without verbose or testing traces */
		rational_logger_mode = REGULAR;
		rational_color_mode = COLOR_MODE_NEVER;
		ASSERT(SUCCESS == match_function_output(m_text(pattern),NULL,capture_pattern_compilation_failure));
		ASSERT(config->ignore_pcre_compiled == NULL);
		ASSERT(config->include_pcre_compiled == NULL);
		ASSERT(config->lock_checksum_pcre_compiled == NULL);
	}

	/* A later option error is reported before regex compilation can start */
	ASSERT(SUCCESS == check_reader("tests/templates/0045_008_10.conf",true,FAILURE,true,"templates/0045_015.txt"));
	ASSERT(config->ignore != NULL && config->ignore[0] != NULL);
	ASSERT(strcmp(config->ignore[0],"(") == 0);
	ASSERT(config->roots.length == 0U);

	call(m_del(pattern));
	RETURN_STATUS;
}

/**
 * @brief Check source decisions from preliminary results without running argp
 * @return SUCCESS when source selection and discarded error results match
 */
static Return check_settings_source_choices(void)
{
	INITTEST;
	char program_name[] = "testitall";
	static const struct {
		int argc;
		error_t preliminary_error;
		const char *explicit_path;
		bool has_other_arguments;
		bool information_mode_requested;
		const char *expected_path;
		bool expected_required;
	} cases[] = {
		{1,EX_OK,NULL,false,false,APP_NAME ".conf",false},
		{2,EX_OK,NULL,true,false,NULL,false},
		{2,EX_OK,NULL,false,false,NULL,false},
		{3,EX_OK,"profile.conf",false,false,"profile.conf",true},
		{2,EX_OK,"",false,false,"",true},
		{2,EX_OK,"--",false,false,"--",true},
		/* Preliminary parsing supplies the last path when --config is repeated */
		{5,EX_OK,"last.conf",false,false,"last.conf",true},
		{1,EINVAL,NULL,false,false,NULL,false},
		{3,ARGP_ERR_UNKNOWN,"profile.conf",false,false,NULL,false},
		{4,EINVAL,"profile.conf",true,false,NULL,false},
		{2,EINVAL,NULL,true,false,NULL,false},
		/* Information requests suppress both automatic and explicit file selection */
		{2,EX_OK,NULL,true,true,NULL,false},
		{4,EX_OK,"profile.conf",true,true,NULL,false},
		{3,EX_OK,"",true,true,NULL,false},
		{4,EINVAL,"profile.conf",true,true,NULL,false}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		config->config_file_path = cases[i].explicit_path;
		ArgumentParseContext context = {
			.config_required = true,
			.has_other_arguments = cases[i].has_other_arguments,
			.information_mode_requested = cases[i].information_mode_requested,
			.diagnostic_state = {.name = program_name,.err_stream = stderr}
		};
		ASSERT(SUCCESS == select_settings_source(cases[i].argc,cases[i].preliminary_error,&context));
		ASSERT(context.config_required == cases[i].expected_required);

		if(cases[i].expected_path != NULL)
		{
			ASSERT(config->config_file_path != NULL);
			ASSERT(strcmp(config->config_file_path,cases[i].expected_path) == 0);
		} else {
			ASSERT(config->config_file_path == NULL);
		}

		if(cases[i].preliminary_error != EX_OK)
		{
			ASSERT(context.has_other_arguments == false);
			ASSERT(context.information_mode_requested == false);
		}
	}

	deliver(status);
}

/**
 * @brief Select settings sources silently and defer preliminary errors to CLI parsing
 * @return Return status for source decisions and empty diagnostic streams
 */
static Return test0045_9(void)
{
	INITTEST;
	ASSERT(SUCCESS == match_function_output(NULL,NULL,check_settings_source_choices));
	RETURN_STATUS;
}

/**
 * @brief Reject an explicit configuration file combined with other arguments
 * @return SUCCESS when the conflict fails without leaving a selected file
 */
static Return check_settings_source_conflict(void)
{
	INITTEST;
	char program_name[] = "testitall";
	config->config_file_path = "missing.conf";
	ArgumentParseContext context = {
		.config_required = true,
		.has_other_arguments = true,
		.diagnostic_state = {.name = program_name,.err_stream = stderr}
	};
	ASSERT(FAILURE == select_settings_source(4,EX_OK,&context));
	ASSERT(config->config_file_path == NULL);
	ASSERT(context.config_required == false);
	deliver(status);
}

/**
 * @brief Check the complete diagnostic for a config and CLI source conflict
 * @return Return status for the expected conflict message and empty stdout
 */
static Return test0045_10(void)
{
	INITTEST;
	m_create(char,pattern,MEMORY_STRING);
	ASSERT(SUCCESS == get_file_content("templates/0045_010_1.txt",pattern));
	ASSERT(SUCCESS == match_function_output(NULL,m_text(pattern),check_settings_source_conflict));
	call(m_del(pattern));
	RETURN_STATUS;
}

/**
 * @brief Apply quoted values while preserving spaces, backslashes, and literal contents
 * @return Return status for quote removal and shared value interpretation
 */
static Return test0045_11(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_reader("tests/templates/0045_011.conf",true,SUCCESS,true,NULL));
	ASSERT(config->roots.length == 3U);
	memory *roots = m_data(memory,conf(roots));
	ASSERT(strcmp(m_text(&roots[0]),"/mnt/my files") == 0);
	ASSERT(strcmp(m_text(&roots[1]),"  spaced path  ") == 0);
	ASSERT(strcmp(m_text(&roots[2]),"$HOME/$(literal)") == 0);

	const char *expected_ignore[] = {
		"^proc/.*","^sys/.*","\\.log$","^\\w+\\s+\\d+$","\"literal quotes\"","^before#after","^literal\"$"
	};
	ASSERT(config->ignore != NULL);

	for(size_t i = 0; i < sizeof(expected_ignore) / sizeof(expected_ignore[0]) && SUCCESS == status; ++i)
	{
		ASSERT(config->ignore[i] != NULL);
		ASSERT(strcmp(config->ignore[i],expected_ignore[i]) == 0);
	}
	ASSERT(config->ignore[7] == NULL);
	ASSERT(config->include != NULL && config->include[0] != NULL);
	ASSERT(strcmp(config->include[0],"^dir with spaces/") == 0);
	ASSERT(config->include[1] == NULL);
	ASSERT(strcmp(confstr(db_primary_file_path),"/tmp/my data.db") == 0);
	ASSERT(strcmp(confstr(db_file_name),"my data.db") == 0);
	ASSERT(config->maxdepth == 3);
	ASSERT(config->db_check_level == QUICK);
	ASSERT(config->update == true);
	ASSERT(config->quiet_ignored == false);
	ASSERT(config->progress == false);
	ASSERT(config->dry_run == true && config->dry_run_with_checksums == true);

	RETURN_STATUS;
}

/**
 * @brief Reject missing or mismatched closing quotes at the original line
 * @return Return status for complete diagnostics and stopping before later records
 */
static Return test0045_12(void)
{
	INITTEST;
	const char *cases[] = {
		"tests/templates/0045_012_1.conf",
		"tests/templates/0045_012_2.conf",
		"tests/templates/0045_012_3.conf",
		"tests/templates/0045_012_4.conf"
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i],true,FAILURE,true,"templates/0045_012_1.txt"));
		ASSERT(config->roots.length == 1U);
		memory *roots = m_data(memory,conf(roots));
		ASSERT(strcmp(m_text(&roots[0]),"first") == 0);
		ASSERT(config->ignore == NULL);
	}

	RETURN_STATUS;
}

/**
 * @brief Preserve comparison paths in file order across repeated keys and continuation lines
 * @return Return status for owned paths, literal values, and configuration cleanup
 */
static Return test0045_13(void)
{
	INITTEST;
	const struct {
		const char *config_template;
		const char *first_path;
		const char *second_path;
	} cases[] = {
		{"tests/templates/0045_013_1.conf","first.db","second.db"},
		{"tests/templates/0045_013_2.conf","first.db","second.db"},
		{"tests/templates/0045_013_3.conf","first db.db","second db.db"},
		{"tests/templates/0045_013_4.conf","first db.db","second db.db"},
		{"tests/templates/0045_013_5.conf","true","false"}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,SUCCESS,true,NULL));
		ASSERT(config->compare == true);
		ASSERT(config->roots.length == 0U);
		ASSERT(config->db_file_paths.length == 2U);
		memory *paths = m_data(memory,conf(db_file_paths));
		ASSERT(strcmp(m_text(&paths[0]),cases[i].first_path) == 0);
		ASSERT(strcmp(m_text(&paths[1]),cases[i].second_path) == 0);
	}

	/* Release the populated array and verify the next configuration starts empty */
	free_config();
	ASSERT(config->db_file_paths.length == 0U);
	ASSERT(SUCCESS == init_config_and_check_output());
	ASSERT(config->compare == false);
	ASSERT(config->db_file_paths.length == 0U);

	RETURN_STATUS;
}

/**
 * @brief Reject an empty comparison path before applying subsequent configuration entries
 * @return Return status for line diagnostics and preservation of earlier paths
 */
static Return test0045_14(void)
{
	INITTEST;
	const struct {
		const char *config_template;
		bool compare;
		size_t path_count;
	} cases[] = {
		{"tests/templates/0045_014_1.conf",false,0U},
		{"tests/templates/0045_014_2.conf",true,1U},
		{"tests/templates/0045_014_3.conf",true,1U},
		{"tests/templates/0045_014_4.conf",true,1U}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,FAILURE,true,"templates/0045_014_1.txt"));
		ASSERT(config->compare == cases[i].compare);
		ASSERT(config->db_file_paths.length == cases[i].path_count);
		ASSERT(config->update == false);

		if(cases[i].path_count != 0U && SUCCESS == status)
		{
			memory *paths = m_data(memory,conf(db_file_paths));
			ASSERT(strcmp(m_text(&paths[0]),"first.db") == 0);
		}
	}

	RETURN_STATUS;
}

/**
 * @brief Parse output format values and retain the last valid selection
 * @return Return status for supported, repeated, empty, and unknown output values
 */
static Return test0045_15(void)
{
	INITTEST;
	const struct {
		const char *config_template;
		OutputMode output;
		Return result;
		const char *error_template;
	} cases[] = {
		{"tests/templates/0045_015_1.conf",OUTPUT_FULL,SUCCESS,NULL},
		{"tests/templates/0045_015_2.conf",OUTPUT_MACHINE,SUCCESS,NULL},
		{"tests/templates/0045_015_3.conf",OUTPUT_FULL,SUCCESS,NULL},
		{"tests/templates/0045_015_4.conf",OUTPUT_MACHINE,SUCCESS,NULL},
		{"tests/templates/0045_015_5.conf",OUTPUT_FULL,FAILURE,"templates/0045_015_5.txt"},
		{"tests/templates/0045_015_6.conf",OUTPUT_FULL,FAILURE,"templates/0045_015_6.txt"},
		{"tests/templates/0045_015_7.conf",OUTPUT_FULL,SUCCESS,NULL},
		{"tests/templates/0045_015_8.conf",OUTPUT_FULL,FAILURE,"templates/0045_015_5.txt"}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,cases[i].result,true,cases[i].error_template));
		ASSERT(config->output == cases[i].output);
	}

	RETURN_STATUS;
}

/**
 * @brief Select logger modes by priority while retaining requested configuration settings
 * @return Return status for logger selection, verbose details, and returning to full output
 */
static Return test0045_16(void)
{
	INITTEST;
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const struct {
		const char *config_template;
		OutputMode output;
		LOGMODES logger_mode;
		bool verbose;
		bool silent;
	} cases[] = {
		{"tests/templates/0045_016_1.conf",OUTPUT_MACHINE,MACHINE,true,false},
		{"tests/templates/0045_016_2.conf",OUTPUT_MACHINE,MACHINE,true,false},
		{"tests/templates/0045_016_3.conf",OUTPUT_MACHINE,SILENT,false,true},
		{"tests/templates/0045_016_4.conf",OUTPUT_MACHINE,SILENT,false,true},
		{"tests/templates/0045_016_5.conf",OUTPUT_MACHINE,SILENT,true,true},
		{"tests/templates/0045_016_6.conf",OUTPUT_MACHINE,SILENT,true,true},
		{"tests/templates/0045_016_7.conf",OUTPUT_MACHINE,MACHINE,false,false},
		{"tests/templates/0045_016_8.conf",OUTPUT_MACHINE,MACHINE,false,false},
		{"tests/templates/0045_016_9.conf",OUTPUT_FULL,VERBOSE,true,false},
		{"tests/templates/0045_016_10.conf",OUTPUT_FULL,initial_logger_mode,false,false},
		{"tests/templates/0045_016_11.conf",OUTPUT_FULL,SILENT,false,true},
		{"tests/templates/0045_016_12.conf",OUTPUT_MACHINE,SILENT,false,true},
		{"tests/templates/0045_016_13.conf",OUTPUT_MACHINE,MACHINE,true,false},
		{"tests/templates/0045_016_14.conf",OUTPUT_MACHINE,SILENT,true,true},
		{"tests/templates/0045_016_15.conf",OUTPUT_MACHINE,SILENT,true,true},
		{"tests/templates/0045_016_16.conf",OUTPUT_FULL,SILENT,true,true},
		{"tests/templates/0045_016_17.conf",OUTPUT_FULL,SILENT,true,true},
		{"tests/templates/0045_016_18.conf",OUTPUT_FULL,VERBOSE,true,false},
		{"tests/templates/0045_016_19.conf",OUTPUT_FULL,initial_logger_mode,false,false},
		{"tests/templates/0045_016_20.conf",OUTPUT_MACHINE,SILENT,true,true},
		{"tests/templates/0045_016_21.conf",OUTPUT_MACHINE,SILENT,true,true}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == check_reader(cases[i].config_template,true,SUCCESS,true,NULL));
		ASSERT(config->output == cases[i].output);
		ASSERT(rational_logger_mode == cases[i].logger_mode);
		ASSERT(config->verbose == cases[i].verbose);
		ASSERT(config->silent == cases[i].silent);
	}

	rational_logger_mode = initial_logger_mode;
	RETURN_STATUS;
}

/**
 * @brief Accept mixed-case named values while retaining literal data
 * @return Return status for named values, paths, and regular expressions
 */
static Return test0045_17(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_reader("tests/templates/0045_017.conf",true,SUCCESS,true,NULL));
	ASSERT(config->output == OUTPUT_MACHINE);
	ASSERT(rational_logger_mode == MACHINE);
	ASSERT(rational_color_mode == COLOR_MODE_NEVER);
	ASSERT(config->db_check_level == QUICK);
	ASSERT(config->dry_run == true && config->dry_run_with_checksums == true);
	ASSERT(config->progress == false && config->quiet_ignored == true);
	ASSERT(config->db_drop_ignored == true && config->db_drop_inaccessible == true);
	ASSERT(config->one_file_system == true);
	ASSERT(config->compare_filter == (CF_CHECKSUM_MISMATCH | CF_FIRST_SOURCE | CF_SECOND_SOURCE));
	ASSERT(strcmp(confstr(db_primary_file_path),"CaSe.DB") == 0);
	ASSERT(config->roots.length == 1U);
	memory *roots = m_data(memory,conf(roots));
	ASSERT(strcmp(m_text(&roots[0]),"MiXeD/Path") == 0);
	ASSERT(config->ignore != NULL && config->ignore[0] != NULL);
	ASSERT(strcmp(config->ignore[0],"^CaSe/Path$") == 0);
	ASSERT(config->include != NULL && config->include[0] != NULL);
	ASSERT(strcmp(config->include[0],"^CaSe/Include$") == 0);
	ASSERT(config->lock_checksum != NULL && config->lock_checksum[0] != NULL);
	ASSERT(strcmp(config->lock_checksum[0],"^CaSe/Locked$") == 0);

	RETURN_STATUS;
}

/**
 * @brief Reset requested output settings and select the startup mode from TESTING
 * @return Return status for repeated initialization and complete startup diagnostics
 */
static Return test0045_18(void)
{
	INITTEST;
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	ASSERT(SUCCESS == get_file_content("templates/0045_033.txt",pattern));
	const struct {
		const char *testing;
		LOGMODES logger_mode;
	} cases[] = {
		{"true",TESTING},
		{"TrUe",TESTING},
		{"false",REGULAR},
		{"",REGULAR},
		{NULL,REGULAR}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && SUCCESS == status; ++i)
	{
		if(cases[i].testing != NULL)
		{
			ASSERT(SUCCESS == set_environment_variable("TESTING",cases[i].testing));
		} else {
			ASSERT(unsetenv("TESTING") == 0);
		}

		/* Initialization must discard all output settings from a previous run */
		free_config();
		config->silent = true;
		config->verbose = true;
		config->output = OUTPUT_MACHINE;
		rational_logger_mode = SILENT;
		ASSERT(SUCCESS == function_capture(init_config,captured_stdout,captured_stderr));
		ASSERT(config->silent == false);
		ASSERT(config->verbose == false);
		ASSERT(config->output == OUTPUT_FULL);
		ASSERT(rational_logger_mode == cases[i].logger_mode);
		ASSERT(captured_stderr->length == 0U);

		if(cases[i].logger_mode == TESTING)
		{
			ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,"templates/0045_033.txt"));
		} else {
			ASSERT(captured_stdout->length == 0U);
		}

		/* Removing higher-priority settings restores the environment's default */
		config->verbose = true;
		ASSERT(SUCCESS == update_logger_mode());
		ASSERT(rational_logger_mode == VERBOSE);
		config->output = OUTPUT_MACHINE;
		ASSERT(SUCCESS == update_logger_mode());
		ASSERT(rational_logger_mode == MACHINE);
		ASSERT(config->verbose == true);
		config->silent = true;
		ASSERT(SUCCESS == update_logger_mode());
		ASSERT(rational_logger_mode == SILENT);
		config->silent = false;
		ASSERT(SUCCESS == update_logger_mode());
		ASSERT(rational_logger_mode == MACHINE);
		config->output = OUTPUT_FULL;
		ASSERT(SUCCESS == update_logger_mode());
		ASSERT(rational_logger_mode == VERBOSE);
		config->verbose = false;
		ASSERT(SUCCESS == update_logger_mode());
		ASSERT(rational_logger_mode == cases[i].logger_mode);
	}

	call(set_environment_variable("TESTING","true"));
	call(update_logger_mode());
	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	RETURN_STATUS;
}

/**
 * @brief Exercise configuration reading and source selection independently of CLI startup
 * @return Return status for the configuration reader and source selection tests
 */
Return test0045(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	config = &test_config;
	ASSERT(SUCCESS == init_config_and_check_output());

	TEST(test0045_1,"Configuration applies values and retains owned strings after reading");
	TEST(test0045_2,"Invalid configuration syntax and keys stop at the first error");
	TEST(test0045_3,"Oversized configuration lines report a parser error");
	TEST(test0045_4,"Configuration absence, opening failures, and read failures remain distinct");
	TEST(test0045_5,"Boolean values apply the last setting and update related modes");
	TEST(test0045_6,"Shared option errors retain their text and configuration line numbers");
	TEST(test0045_7,"Regex values retain their order and remain available for working compilation");
	TEST(test0045_8,"Regex errors are deferred until working compilation, after configuration parsing");
	TEST(test0045_9,"Settings source selection is silent and discards preliminary parsing errors");
	TEST(test0045_10,"Explicit configuration and other arguments produce a source conflict");
	TEST(test0045_11,"Quoted configuration values preserve literal contents and apply normal value rules");
	TEST(test0045_12,"Unclosed configuration quotes report their line and stop reading");
	TEST(test0045_13,"Comparison paths retain their order and literal contents until configuration cleanup");
	TEST(test0045_14,"Empty comparison paths report their line and stop configuration reading");
	TEST(test0045_15,"Output format values are validated and the last selection wins");
	TEST(test0045_16,"Output settings retain their values and select logger modes by priority");
	TEST(test0045_17,"Configuration ignores named value case while preserving literal data");
	TEST(test0045_18,"Logger priorities restore startup defaults and initialization clears prior settings");

	/* Restore the runner's configuration and output policy after any test result */
	free_config();
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;

	RETURN_STATUS;
}
