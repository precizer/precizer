#include "precizer.h"
#include <sysexits.h>

/**
 * @brief Parse command-line arguments and populate global runtime configuration
 */

/* Program documentation text used by argp */
static char doc[] =
        "\n" BOLD APP_NAME RESET ": data integrity verification for file systems of any scale\n\n"
        BOLD APP_NAME RESET " is a lightweight, high-performance CLI tool written in pure C. It’s designed for file integrity verification and comparison, making it especially useful for validating synchronization results. The program traverses directory trees and builds a database of files and their checksums for fast, repeatable comparisons.\n"
        "\n"
        "Built for embedded systems and large-scale clustered environments, " BOLD APP_NAME RESET " detects synchronization drift by comparing files and checksums across sources. It can also analyze historical changes by comparing databases captured from the same source at different points in time.\n"
        "\n"
        "With love for Ukraine\n"
        "\vSIMPLE EXAMPLE\n"
        "\n"
        "Use this workflow to verify that two mounted directory trees are equivalent.\n"
        "\n"
        "Assume the source trees are mounted at " YELLOW "/mnt1" RESET " and " YELLOW "/mnt2" RESET ".\n"
        "\n"
        "1. On machine " YELLOW "'host1'" RESET ", run:\n"
        "\n"
        "   " BOLDGREEN "$ " APP_NAME " --progress /mnt1" RESET "\n"
        "\n"
        "This traversal scans " YELLOW "/mnt1" RESET " recursively and creates " YELLOW "host1.db" RESET " in the current directory.\n"
        "The " BOLD "--progress" RESET " option reports processed data volume and file count.\n"
        "\n"
        "2. On machine " YELLOW "'host2'" RESET ", run:\n"
        "\n"
        "   " BOLDGREEN "$ " APP_NAME " --progress /mnt2" RESET "\n"
        "\n"
        "This creates " YELLOW "host2.db" RESET " in the current directory.\n"
        "\n"
        "3. Copy " YELLOW "host1.db" RESET " and " YELLOW "host2.db" RESET " to one machine, then run:\n"
        "\n"
        "   " BOLDGREEN "$ " APP_NAME " --compare host1.db host2.db" RESET "\n"
        "\n"
        "Output reports:\n"
        "\n"
        "* Files present on " YELLOW "'host1'" RESET " but missing on " YELLOW "'host2'" RESET ", and vice versa.\n"
        "* Files present on both hosts whose SHA512 checksums do not match.\n"
        "\n"
        "Database paths are stored as relative paths only.\n"
        "For example, " YELLOW "/mnt1/abc/def/aaa.txt" RESET " is stored as " YELLOW "abc/def/aaa.txt" RESET ". "
        "The same relative-path rule applies to " YELLOW "/mnt2/abc/def/aaa.txt" RESET ", enabling direct cross-source comparison.\n"
        "\n"
        "See the project README for additional technical details.";

/* Positional-argument description for argp */
static char args_doc[] = "PATH";

static bool information_mode_requested = false;

/* Supported command-line options for argp */
static struct argp_option options[] = {
	{ 0,0,0,0,"Locked Checksum Protection:",3},
	{"lock-checksum",'k',"PCRE2_REGEXP",0,"Relative path to be treated as immutable archival data. PCRE2 regular expressions can be used to "
	 "select files or directories whose checksums are written once to the database and never updated "
	 "again. If no matching files exist in the database yet, their entries and checksums will still be "
	 "created normally when they are scanned for the first time. All paths for the regular expression "
	 "must be specified as relative, the same way as for the "
	 BOLD "--ignore" RESET " option. For these entries, the "
	 BOLD "--update" RESET " option will not recalculate checksums; any difference in file size, timestamps, or content will "
	 "always be reported as data corruption, and no replacement checksum will be generated. Multiple "
	 "regular expressions can be specified using multiple "
	 BOLD "--lock-checksum" RESET " options.\n"
	 "Example:\n"
	 BOLD APP_NAME " --update --lock-checksum=\"^archive/2077/.*\" /mnt/storage" RESET,0},
	{"rehash-locked",'r',0,0,"Force a full SHA512 rehash for every file that is already stored "
	 "in the database and protected by any "
	 BOLD "--lock-checksum" RESET " pattern. "
	 "This option must always be used together with "
	 BOLD "--lock-checksum" RESET "; it has no effect otherwise. During an update pass, "
	 "every locked entry found on disk is read again, its checksum is recomputed, and the "
	 "result is compared with the value stored in the database. This provides an extra "
	 "validation layer for immutable archives at the cost of additional I/O and CPU time, "
	 "ensuring that frozen records are validated even when file size or timestamps alone are "
	 "not sufficient indicators. "
	 "The option purposefully ignores "
	 BOLD "--watch-timestamps" RESET " — timestamp tracking can be on or off, the rehash will "
	 "still take place. If the recomputed checksum and file size match the database entry, "
	 "the record stays consistent but differing ctime/mtime values in the database are "
	 "synchronized with the on-disk timestamps. If the checksum differs, the file is "
	 "reported as corrupted just like any other locked entry.\n"
	 "Example:\n"
	 BOLD APP_NAME " --update --lock-checksum=\"^archive/2024/.*\" --rehash-locked /mnt/storage" RESET,0},
	{ 0,0,0,0,"Build database options:",2},
	{ 0,0,0,0,"Path Filtering and Ignore Policy:",4},
	{"ignore",'e',"PCRE2_REGEXP",0,"Relative path to ignore. PCRE2 regular expressions could be used to specify "
	 "a pattern to ignore files or directories. Attention! All paths for the regular expression must be specified as relative. To understand what a relative path looks like, just run traverses without the "
	 BOLD "--ignore" RESET " option and look how the terminal will display "
	 "relative paths that are written to the database.\n"
	 "Example:\n"
	 BOLD APP_NAME " --ignore=\"^diff2/1/.*\" tests/fixtures/diffs" RESET "\n"
	 "In this example, the starting path for the traversing "
	 "is ./tests/fixtures/diffs and the relative path to ignore will "
	 "be ./tests/fixtures/diffs/diff2/1/ and all subdirectories (/.*).\n"
	 "Multiple regular expressions for ignore could be specified using many "
	 BOLD "--ignore" RESET " options at once.\n"
	 "Example:\n"
	 BOLD APP_NAME " --ignore=\"diff2/1/.*\" --ignore=\"diff2/2/.*\" tests/fixtures/diffs" RESET "\n"
	 "With " BOLD "--compare" RESET ", ignored paths are treated as out of scope for reported differences and equality summaries.",4 },
	{"include",'i',"PCRE2_REGEXP",0,"Relative path to be included. PCRE2 regular expressions. Include these relative paths even if they were excluded via the " BOLD "--ignore" RESET " option. Multiple regular expressions could be specified. With " BOLD "--compare" RESET ", included paths are restored back into the reported comparison scope.",4 },
	{"db-drop-ignored",'C',0,0,"The database is protected from accidental changes by default. The option " BOLD "--db-drop-ignored" RESET " must be specified additionally in order to remove from the database mention of files that matches the regular expression passed through the " BOLD "--ignore=PCRE2_REGEXP" RESET " option(s).",3},
	{"db-clean-ignored",'C',0,OPTION_ALIAS | OPTION_HIDDEN,0,3}, // This legacy can be removed in 2036 (10-year Long-Term Support)
	{"db-drop-inaccessible",'X',0,0,"Allow dropping database records for files that are inaccessible due to permission errors. By default, such paths are reported as \"inaccessible\" and their DB records are kept to avoid accidental loss when permissions change. This option is effective only with " BOLD "--update" RESET ".\n"
	 "Example:\n"
	 BOLD APP_NAME " --update --db-drop-inaccessible /mnt/storage" RESET,2},
	{"drop-inaccessible",'X',0,OPTION_ALIAS | OPTION_HIDDEN,0,0}, // This legacy can be removed in 2036 (10-year Long-Term Support)
	{"watch-timestamps",'T',0,0,"Consider file metadata changes (creation and modification timestamps) in addition to file size when detecting changes. By default, only file size changes trigger rescanning. When this option is enabled, any changes to file timestamps or size will cause the file to be rescanned and its checksum updated in the primary database.",0},
	{"maxdepth",'m',"NUMBER",0,"Recursion depth limit. The depth of the traversal, numbered from 0 to N, where a file could be found. Representing the maximum of the starting point (from root) of the traversal. The root itself is numbered 0. " BOLD "--maxdepth=0" RESET " completely disable recursion.",0},
	{"dry-run",'n',"MODE",OPTION_ARG_OPTIONAL,"Perform a trial run with no changes made. The option will not affect " BOLD "--compare" RESET ". "
	 "Supported mode: " BOLD "--dry-run=with-checksums" RESET " (read files and calculate checksums during dry run).",0},
	{"start-device-only",'o',0,0,"This option prevents directory traversal from descending into directories that have a different device number than the file from which the descent began.",0 },
	{"force",'f',0,0,"Use this option only in case when the PATHs that were written into the database as a result of the last scanning really need to be renewed. Warning! If this option will be used in incorrect way, information about files and their checksums against the database would be lost.",0},
	{"update",'u',0,0,"Updates the database to reflect file system changes (new, modified and deleted files). Must be used with the same initial PATH that was used when creating the database, as existing records will be replaced with data from the specified location. This option modifies database consistency. Use with caution, especially in automated scripts, as incorrect usage may lead to loss of file checksums and metadata.",0 },
	{"database",'d',"FILE",0,"Database filename. Defaults to ${HOST}.db, where HOST is the local hostname.",0 },
	{"check-level",'l',"FULL|QUICK",0,"Select database validation level: 'quick' for basic structure check, 'full' (default) for comprehensive integrity verification.",0 },
	{ 0,0,0,0,"Compare databases options:",1},
	{"compare",'c',0,0,"Compare two databases from different sources. Requires two additional arguments specifying paths to database files, e.g.:\n" BOLD APP_NAME " --compare database1.db database2.db" RESET "\n"
	 "When combined with " BOLD "--ignore/--include" RESET ", the reported differences and equality summaries are limited to the filtered relative-path scope.",0 },
	{"compare-filter",'F',"checksum-mismatch|first-source|second-source",0,
	 "Filter output categories for " BOLD "--compare" RESET ". "
	 "Supported values: "
	 BOLD "checksum-mismatch" RESET ", "
	 BOLD "first-source" RESET ", "
	 BOLD "second-source" RESET ". "
	 "The option can be specified multiple times in any combination.",0},
	{ 0,0,0,0,"Visualizations options:",-1},
	{"silent",'s',0,0,"Don't produce any output. With " BOLD "--compare" RESET ", only paths with differences are shown, and category headings remain visible when multiple compare categories are active.",0 },
	{"quiet-ignored",'q',0,0,"Suppress per-file log lines for paths filtered by " BOLD "--ignore/--include" RESET ". This helps keep program logs free of extra messages once ignore regular expressions are tuned and stable in use. Other warnings and errors remain visible.",0 },
	{"verbose",'v',0,0,"Produce verbose output.",0 },
	{"progress",'p',0,0,"Enabling this option displays progress information but requires an initial count of files and the space they occupy to estimate execution time. The program first traverses all specified directories, counting files, folders, and symlinks before proceeding with file analysis. This initial traversal may take a significant amount of time. It is strongly recommended not to use this option when calling the program from a script.",0 },
	{"help",'h',0,0,"Give this help list",-1 },
	{0,'?',0,OPTION_ALIAS,0,-1 },
	{"usage",'z',0,0,"Give a short usage message",-1 },
	{"version",'V',0,0,"Print program version",-1 },
	{0}
};

/**
 * @brief Convert an argp parse error code into human-readable text
 * @param parse_error Error code returned by argp_parse
 * @return Pointer to a static descriptive string
 */
static const char *argp_error_to_text(const error_t parse_error)
{
	if(parse_error == EX_USAGE)
	{
		return("command line usage error");
	}

	if(parse_error == ARGP_ERR_UNKNOWN)
	{
		return("unknown argument parsing error");
	}

	const char *message = strerror(parse_error);

	if(message == NULL || message[0] == '\0')
	{
		return("unknown error");
	}

	return(message);
}

/**
 * @brief Handle a single argp parser event
 * @param key Current option key or argp event key
 * @param arg Optional value for the current option
 * @param state Current argp parser state
 * @return EX_OK on success, EX_USAGE for invalid usage, or errno-style error code
 */
static error_t parse_opt(
	int               key,
	char              *arg,
	struct argp_state *state)
{
	char *ptr = NULL;
	long int argument_value = -1;

	switch(key)
	{
		case 'd':
		{
			// Store full path to the DB file
			if(CRITICAL & m_copy_string(conf(db_primary_file_path),arg))
			{
				argp_failure(state,0,ENOMEM,"ERROR: Memory allocation for db_file_path failed");
				return(ENOMEM);
			}

			// Store only the DB file basename
			char *tmp = strdup(arg);

			if(tmp == NULL)
			{
				(void)m_del(conf(db_primary_file_path));
				argp_failure(state,0,ENOMEM,"ERROR: Memory allocation for db_file_name failed");
				return(ENOMEM);
			}

			const char *db_file_basename = basename(tmp);

			if(db_file_basename == NULL)
			{
				free(tmp);
				(void)m_del(conf(db_primary_file_path));
				argp_failure(state,0,0,"ERROR: Failed to determine database base name from path '%s'",arg);
				return(EINVAL);
			}

			if(CRITICAL & m_copy_string(conf(db_file_name),db_file_basename))
			{
				free(tmp);
				(void)m_del(conf(db_primary_file_path));
				argp_failure(state,0,ENOMEM,"ERROR: Memory allocation for db_file_name failed");
				return(ENOMEM);
			}

			free(tmp);
			break;
		}
		case 'e':
			(void)add_string_to_array(&config->ignore,arg);
			break;
		case 'n':
			config->dry_run = true;

			if(arg != NULL)
			{
				if(0 == strcasecmp(arg,"with-checksums"))
				{
					config->dry_run_with_checksums = true;

				} else {
					argp_failure(state,0,0,"ERROR: Unsupported --dry-run mode '%s'. Supported mode: with-checksums. See --help for more information",arg);
					return(EINVAL);
				}
			}
			break;
		case 'i':
			(void)add_string_to_array(&config->include,arg);
			// Track that at least one --include pattern was provided
			config->include_specified = true;
			break;
		case 'k':
			(void)add_string_to_array(&config->lock_checksum,arg);
			break;
		case 'r':
			config->rehash_locked = true;
			break;
		case 'c':
			config->compare = true;
			break;
		case 'F':
			if(arg != NULL && 0 == strcmp(arg,"checksum-mismatch"))
			{
				config->compare_filter |= CF_CHECKSUM_MISMATCH;
			} else if(arg != NULL && 0 == strcmp(arg,"first-source")){
				config->compare_filter |= CF_FIRST_SOURCE;
			} else if(arg != NULL && 0 == strcmp(arg,"second-source")){
				config->compare_filter |= CF_SECOND_SOURCE;
			} else {
				argp_failure(state,0,0,"ERROR: Unsupported --compare-filter value '%s'. Supported values: checksum-mismatch, first-source, second-source. See --help for more information",arg == NULL ? "" : arg);
				return(EINVAL);
			}
			break;
		case 'o':
			config->start_device_only = true;
			break;
		case 'C':
			config->db_drop_ignored = true;
			break;
		case 'X':
			config->db_drop_inaccessible = true;
			break;
		case 'm':
			argument_value = strtol(arg,&ptr,10);

			// Accept only non-negative integer values that fit into short int
			// and reject strings with trailing non-numeric characters
			if(ptr != arg && argument_value >= 0 && argument_value <= 32767 && *ptr == '\0')
			{
				config->maxdepth = (short int)argument_value;
			} else {
				argp_failure(state,0,0,"ERROR: Wrong --maxdepth (-m) value. Should be an integer from 0 to 32767. See --help for more information");
				return(EINVAL);
			}
			break;
		case 'p':
			config->progress = true;
			/*
			 * Progress output can push important warning lines out of the visible terminal area.
			 * Enable final replay before traversal starts so remembered warnings collected later are shown again near exit
			 */
			config->show_remembered_messages_at_exit = true;
			break;
		case 'T':
			config->watch_timestamps = true;
			break;
		case 'u':
			config->update = true;
			break;
		case 'f':
			config->force = true;
			break;
		case 'l':
			if(0 == strcasecmp(arg,"QUICK"))
			{
				config->db_check_level = QUICK;
			} else if(0 == strcasecmp(arg,"FULL")){
				config->db_check_level = FULL;
			} else {
				argp_failure(state,0,0,"ERROR: Unsupported --check-level value '%s'. Supported values: FULL or QUICK",arg);
				return(EINVAL);
			}
			break;
		case 's':
			// Set global logger mode
			rational_logger_mode = SILENT;
			break;
		case 'q':
			config->quiet_ignored = true;
			break;
		case 'v':
			// Set global logger mode
			rational_logger_mode = VERBOSE;
			config->verbose = true;
			break;
		case 'h':
		case '?':
			information_mode_requested = true;
			argp_state_help(state,state->out_stream,ARGP_HELP_STD_HELP & ~(ARGP_HELP_EXIT_OK | ARGP_HELP_EXIT_ERR));
			break;
		case 'V':
			information_mode_requested = true;
			break;
		case 'z':
			information_mode_requested = true;
			argp_state_help(state,state->out_stream,ARGP_HELP_USAGE);
			break;
		case ARGP_KEY_NO_ARGS:
			if(information_mode_requested == true)
			{
				break;
			}

			if(state->argc == 1)
			{
				information_mode_requested = true;
				argp_usage(state);
				break;
			}

			argp_usage(state);
			return(EX_USAGE);
			break;
		case ARGP_KEY_ARG:
		{
			Return status;

			/*
			 * This event receives one positional command-line argument.
			 * Argp normally processes options before positional arguments, so
			 * `--compare` has already selected the intended mode by this point.
			 *
			 * In compare mode, positional arguments are database file paths.
			 * In normal scan mode, positional arguments are filesystem roots
			 * that will be traversed later
			 */
			if(config->compare == true)
			{
				status = m_string_array_append(conf(db_file_paths),char,arg);
			} else {
				status = m_string_array_append(conf(roots),char,arg);
			}

			if((TRIUMPH & status) == 0)
			{
				argp_failure(state,0,ENOMEM,"ERROR: Memory allocation for positional argument failed");
				return(ENOMEM);
			}
			break;
		}
		case ARGP_KEY_END:
			if(information_mode_requested == true)
			{
				break;
			}

			if(config->compare_filter != CF_NONE_SPECIFIED && config->compare == false)
			{
				argp_failure(state,0,0,"ERROR: --compare-filter can only be used together with --compare. See --help for more information");
				return(EX_USAGE);
			}

			if(config->compare == true)
			{
				if(config->db_file_paths.length < 2)
				{
					argp_failure(state,0,0,"ERROR: Too few arguments\n--compare require two arguments with paths to database files. See --help for more information");
					return(EX_USAGE);
				} else if(config->db_file_paths.length > 2){
					argp_failure(state,0,0,"ERROR: Too many arguments\n--compare require just two arguments with paths to database files. See --help for more information");
					return(EX_USAGE);
				}
			}

			if(config->db_drop_inaccessible == true && config->update == false)
			{
				argp_failure(state,0,0,"WARNING: --db-drop-inaccessible has no effect without --update; records for inaccessible paths will be kept in the database");
			}

			if(config->rehash_locked == true && config->lock_checksum == NULL)
			{
				argp_failure(state,0,0,"WARNING: --rehash-locked has no effect without --lock-checksum");
			}
			break;
		default:
			return(ARGP_ERR_UNKNOWN);
	}

	return(EX_OK);
}

/* argp parser definition */
static struct argp argp = {
	options,parse_opt,args_doc,doc,0,0,0
};

/* Keep diagnostics attributed to parse_arguments() after moving their bodies into helpers */
#define parse_arguments_slog(level,...) rational_logger((level),__FILE__,__LINE__,"parse_arguments",__VA_ARGS__)

/**
 * @brief Show parsed configuration details for TESTING logger output
 *
 * @details
 * Test templates use these lines as stable key-value diagnostics. The function
 * prints only options that were explicitly set or values that are useful for
 * validating argument parsing behavior
 */
static void parse_arguments_show_testing_diagnostics(void)
{
	parse_arguments_slog(TESTING,"rational_logger_mode=%s\n",rational_reconvert(rational_logger_mode));

	if(config->roots.length != 0)
	{
		bool first_root = true;

		parse_arguments_slog(TESTING,"argument:roots=");

		m_string_array_foreach(conf(roots),root)
		{
			parse_arguments_slog(TESTING|UNDECOR,first_root == true ? "%s" : ", %s",m_text(root));
			first_root = false;
		}
		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	// Print configured database path when available
	if(conf(db_primary_file_path)->string_length > 0)
	{
		parse_arguments_slog(TESTING,"argument:database=%s\n",confstr(db_primary_file_path));
	}

	// Print configured database file name when available
	if(conf(db_file_name)->string_length > 0)
	{
		parse_arguments_slog(TESTING,"argument:db_file_name=%s\n",confstr(db_file_name));
	}

	if(config->db_file_paths.length != 0)
	{
		bool first_db_file_path = true;

		parse_arguments_slog(TESTING,"argument:db_file_paths=");

		m_string_array_foreach(conf(db_file_paths),db_file_path)
		{
			parse_arguments_slog(TESTING|UNDECOR,first_db_file_path == true ? "%s" : ", %s",m_text(db_file_path));
			first_db_file_path = false;
		}
		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	if(config->db_file_names != NULL)
	{
		parse_arguments_slog(TESTING,"argument:db_file_names=");

		for(int i = 0; config->db_file_names[i]; i++)
		{
			parse_arguments_slog(TESTING|UNDECOR,i == 0 ? "%s" : ", %s",config->db_file_names[i]);
		}
		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	if(config->ignore != NULL)
	{
		parse_arguments_slog(TESTING,"argument:ignore=");

		// Print string-array contents
		for(int i = 0; config->ignore[i] != NULL; ++i)
		{
			parse_arguments_slog(TESTING|UNDECOR,i == 0 ? "%s" : ", %s",config->ignore[i]);
		}
		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	if(config->include != NULL)
	{
		parse_arguments_slog(TESTING,"argument:include=");

		// Print string-array contents
		for(int i = 0; config->include[i] != NULL; ++i)
		{
			parse_arguments_slog(TESTING|UNDECOR,i == 0 ? "%s" : ", %s",config->include[i]);
		}
		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	if(config->lock_checksum != NULL)
	{
		parse_arguments_slog(TESTING,"argument:lock-checksum=");

		// Print string-array contents
		for(int i = 0; config->lock_checksum[i] != NULL; ++i)
		{
			parse_arguments_slog(TESTING|UNDECOR,i == 0 ? "%s" : ", %s",config->lock_checksum[i]);
		}
		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	if(config->db_check_level != FULL)
	{
		parse_arguments_slog(TESTING,"argument:check-level=%s\n",config->db_check_level == QUICK ? "QUICK" : "FULL");
	}

	if(config->maxdepth > 0)
	{
		parse_arguments_slog(TESTING,"argument:maxdepth=%d\n",config->maxdepth);
	}

	if(config->verbose)
	{
		parse_arguments_slog(TESTING,"argument:verbose=%s\n",config->verbose ? "yes" : "no");
	}

	if(config->quiet_ignored)
	{
		parse_arguments_slog(TESTING,"argument:quiet-ignored=%s\n",config->quiet_ignored ? "yes" : "no");
	}

	if(config->watch_timestamps)
	{
		parse_arguments_slog(TESTING,"argument:watch-timestamps=%s\n",config->watch_timestamps ? "yes" : "no");
	}

	if(config->rehash_locked == true)
	{
		parse_arguments_slog(TESTING,"argument:rehash-locked=%s\n",config->rehash_locked ? "yes" : "no");
	}

	if(config->force)
	{
		parse_arguments_slog(TESTING,"argument:force=%s\n",config->force ? "yes" : "no");
	}

	if(config->update)
	{
		parse_arguments_slog(TESTING,"argument:update=%s\n",config->update ? "yes" : "no");
	}

	if(config->progress)
	{
		parse_arguments_slog(TESTING,"argument:progress=%s\n",config->progress ? "yes" : "no");
	}

	if(config->compare)
	{
		parse_arguments_slog(TESTING,"argument:compare=%s\n",config->compare ? "yes" : "no");
	}

	if(config->compare_filter)
	{
		bool first_compare_filter = true;

		parse_arguments_slog(TESTING,"argument:compare-filter=");

		if((config->compare_filter & CF_CHECKSUM_MISMATCH) != 0u)
		{
			parse_arguments_slog(TESTING|UNDECOR,"%schecksum-mismatch",first_compare_filter ? "" : ", ");
			first_compare_filter = false;
		}

		if((config->compare_filter & CF_FIRST_SOURCE) != 0u)
		{
			parse_arguments_slog(TESTING|UNDECOR,"%sfirst-source",first_compare_filter ? "" : ", ");
			first_compare_filter = false;
		}

		if((config->compare_filter & CF_SECOND_SOURCE) != 0u)
		{
			parse_arguments_slog(TESTING|UNDECOR,"%ssecond-source",first_compare_filter ? "" : ", ");
		}

		parse_arguments_slog(TESTING|UNDECOR,"\n");
	}

	if(config->db_drop_ignored)
	{
		parse_arguments_slog(TESTING,"argument:db-drop-ignored=%s\n",config->db_drop_ignored ? "yes" : "no");
	}

	if(config->db_drop_inaccessible)
	{
		parse_arguments_slog(TESTING,"argument:db-drop-inaccessible=%s\n",config->db_drop_inaccessible ? "yes" : "no");
	}

	if(config->dry_run)
	{
		parse_arguments_slog(TESTING,"argument:dry-run=%s\n",config->dry_run_with_checksums ? "with-checksums" : "yes");
	}

	if(config->start_device_only)
	{
		parse_arguments_slog(TESTING,"argument:start-device-only=%s\n",config->start_device_only ? "yes" : "no");
	}
}

/**
 * @brief Show parsed configuration details for verbose user output
 *
 * @details
 * Verbose output is a compact, single-line configuration summary for people
 * running the program interactively. It mirrors the TESTING diagnostics while
 * preserving the existing user-facing formatting
 */
static void parse_arguments_show_verbose_diagnostics(void)
{
	parse_arguments_slog(VERBOSE,"Configuration: ");
	parse_arguments_slog(VERBOSE|UNDECOR,"rational_logger_mode=%s\n",rational_reconvert(rational_logger_mode));

	if(config->roots.length != 0)
	{
		bool first_root = true;

		parse_arguments_slog(VERBOSE|UNDECOR,"roots=");

		m_string_array_foreach(conf(roots),root)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,first_root == true ? "%s" : ", %s",m_text(root));
			first_root = false;
		}
		parse_arguments_slog(VERBOSE|UNDECOR,"; ");
	}

	// Print configured database path when available
	if(conf(db_primary_file_path)->string_length > 0)
	{
		parse_arguments_slog(VERBOSE|UNDECOR,"database=%s; ",confstr(db_primary_file_path));
	}

	// Print configured database file name when available
	if(conf(db_file_name)->string_length > 0)
	{
		parse_arguments_slog(VERBOSE|UNDECOR,"db_file_name=%s; ",confstr(db_file_name));
	}

	if(config->db_file_paths.length != 0)
	{
		bool first_db_file_path = true;

		parse_arguments_slog(VERBOSE|UNDECOR,"db_file_paths=");

		m_string_array_foreach(conf(db_file_paths),db_file_path)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,first_db_file_path == true ? "%s" : ", %s",m_text(db_file_path));
			first_db_file_path = false;
		}
		parse_arguments_slog(VERBOSE|UNDECOR,"; ");
	}

	if(config->db_file_names != NULL)
	{
		parse_arguments_slog(VERBOSE|UNDECOR,"db_file_names=");

		for(int i = 0; config->db_file_names[i]; i++)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,i == 0 ? "%s" : ", %s",config->db_file_names[i]);
		}
		parse_arguments_slog(VERBOSE|UNDECOR,"; ");
	}

	if(config->ignore != NULL)
	{
		parse_arguments_slog(VERBOSE|UNDECOR,"ignore=");

		// Print string-array contents
		for(int i = 0; config->ignore[i] != NULL; ++i)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,i == 0 ? "%s" : ", %s",config->ignore[i]);
		}
		parse_arguments_slog(VERBOSE|UNDECOR,"; ");
	}

	if(config->include != NULL)
	{
		parse_arguments_slog(VERBOSE|UNDECOR,"include=");

		// Print string-array contents
		for(int i = 0; config->include[i] != NULL; ++i)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,i == 0 ? "%s" : ", %s",config->include[i]);
		}
		parse_arguments_slog(VERBOSE|UNDECOR,"; ");
	}

	if(config->lock_checksum != NULL)
	{
		parse_arguments_slog(VERBOSE|UNDECOR,"lock-checksum=");

		// Print string-array contents
		for(int i = 0; config->lock_checksum[i] != NULL; ++i)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,i == 0 ? "%s" : ", %s",config->lock_checksum[i]);
		}
		parse_arguments_slog(VERBOSE|UNDECOR,"; ");
	}

	const char *dry_run_mode = "no";

	if(config->dry_run == true)
	{
		if(config->dry_run_with_checksums == true)
		{
			dry_run_mode = "with-checksums";
		} else {
			dry_run_mode = "yes";
		}
	}

	parse_arguments_slog(VERBOSE|UNDECOR,"verbose=%s; maxdepth=%d; silent=no; quiet-ignored=%s; force=%s; update=%s; watch-timestamps=%s; rehash-locked=%s; progress=%s; compare=%s, db-drop-ignored=%s, db-drop-inaccessible=%s, dry-run=%s, start-device-only=%s, check-level=%s, rational_logger_mode=%s",
		config->verbose ? "yes" : "no",
		config->maxdepth,
		config->quiet_ignored ? "yes" : "no",
		config->force ? "yes" : "no",
		config->update ? "yes" : "no",
		config->watch_timestamps ? "yes" : "no",
		config->rehash_locked ? "yes" : "no",
		config->progress ? "yes" : "no",
		config->compare ? "yes" : "no",
		config->db_drop_ignored ? "yes" : "no",
		config->db_drop_inaccessible ? "yes" : "no",
		dry_run_mode,
		config->start_device_only ? "yes" : "no",
		config->db_check_level == QUICK ? "QUICK" : "FULL",
		rational_reconvert(rational_logger_mode));

	if(config->compare_filter)
	{
		bool first_compare_filter = true;

		parse_arguments_slog(VERBOSE|UNDECOR,"; compare-filter=");

		if((config->compare_filter & CF_CHECKSUM_MISMATCH) != 0u)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,"%schecksum-mismatch",first_compare_filter ? "" : ", ");
			first_compare_filter = false;
		}

		if((config->compare_filter & CF_FIRST_SOURCE) != 0u)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,"%sfirst-source",first_compare_filter ? "" : ", ");
			first_compare_filter = false;
		}

		if((config->compare_filter & CF_SECOND_SOURCE) != 0u)
		{
			parse_arguments_slog(VERBOSE|UNDECOR,"%ssecond-source",first_compare_filter ? "" : ", ");
		}
	}

	parse_arguments_slog(VERBOSE|UNDECOR,"\n");
}

#undef parse_arguments_slog

/**
 * @brief Parse command-line arguments and finalize parse-related configuration state
 *
 * In normal scan mode, positional arguments become traversal roots. In
 * `--compare` mode, the two positional arguments become database paths and
 * their file names are used in user-facing messages
 *
 * For example, `precizer src tests` creates two traversal roots, while
 * `precizer --compare first.db second.db` creates two compare database paths
 *
 * @param argc Number of CLI arguments
 * @param argv CLI argument vector
 * @return SUCCESS for normal mode, INFO|HALTED for informational mode, or failure flags on errors
 */
Return parse_arguments(
	const int argc,
	char      *argv[])
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	unsigned int parse_flags = ARGP_NO_EXIT | ARGP_NO_HELP;

	information_mode_requested = false;

	/* Parse arguments and route each parser event through parse_opt */
	error_t parse_error = argp_parse(&argp,argc,argv,parse_flags,0,0);

	if(parse_error != EX_OK)
	{
		status = FAILURE;
	} else if(information_mode_requested == true){
		provide(INFO | HALTED);
	}

	/*
	 * Normal scan mode stores positional arguments as traversal roots.
	 * Normalize those roots after argp has finished collecting them, so later
	 * code can compare paths without treating `dir` and `dir/` as different
	 * user input
	 */
	if((SUCCESS & status) && config->compare == false && config->roots.length != 0)
	{
		m_string_array_foreach(conf(roots),root)
		{
			run(remove_trailing_slash(root));

			if((SUCCESS & status) == 0)
			{
				break;
			}
		}
	}

	/*
	 * Compare mode stores positional arguments as database file paths instead
	 * of traversal roots. Normalize each database path, then keep its basename
	 * separately so compare output can show friendly source names like
	 * `first.db` and `second.db`
	 */
	if((SUCCESS & status) && config->compare == true)
	{
		if(config->db_file_paths.length != 0)
		{
			m_string_array_foreach(conf(db_file_paths),db_file_path)
			{
				run(remove_trailing_slash(db_file_path));

				if((SUCCESS & status) == 0)
				{
					break;
				}

				const char *db_file_path_text = m_text(db_file_path);

				// Duplicate the path because basename may modify the buffer
				char *tmp = strdup(db_file_path_text);

				if(tmp == NULL)
				{
					report("Failed to duplicate string: %s",db_file_path_text);
					status = FAILURE;
					break;
				}

				// Resolve basename and handle NULL result
				const char *db_file_basename = basename(tmp);

				if(db_file_basename == NULL)
				{
					report("basename failed for path: %s",tmp);
					free(tmp);
					status = FAILURE;
					break;
				}

				status = add_string_to_array(&config->db_file_names,db_file_basename);
				free(tmp);

				if((SUCCESS & status) == 0)
				{
					break;
				}
			}
		}
	}

	if(parse_error != 0)
	{
		slog(ERROR,"Argument parsing failed with code %d (%s)\n",parse_error,argp_error_to_text(parse_error));
		status = FAILURE;
	}

	if(CRITICAL & status)
	{
		provide(status);
	}

	/* Show key-value diagnostics consumed by test templates */
	parse_arguments_show_testing_diagnostics();

	/* Show compact configuration summary for verbose user output */
	parse_arguments_show_verbose_diagnostics();

	slog(TRACE,"Argument parsing is finished\n");

	provide(status);
}
