#include "precizer.h"
#include <stdarg.h>
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
static char args_doc[] = "[PATH...]";

static bool information_mode_requested = false;

/* Supported command-line options for argp */
const struct argp_option options[] = {
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
	{"watch-timestamps",'T',0,0,"Also recalculate checksums when metadata change time (ctime) or allocated block count changes. By default, regular files are rehashed when their size or content modification time (mtime) changes.",0},
	{"maxdepth",'m',"NUMBER",0,"Recursion depth limit. The depth of the traversal, numbered from 0 to N, where a file could be found. Representing the maximum of the starting point (from root) of the traversal. The root itself is numbered 0. " BOLD "--maxdepth=0" RESET " completely disable recursion.",0},
	{"dry-run",'n',"MODE",OPTION_ARG_OPTIONAL,"Perform a trial run with no changes made. The option will not affect " BOLD "--compare" RESET ". "
	 "Supported mode: " BOLD "--dry-run=with-checksums" RESET " (read files and calculate checksums during dry run).",0},
	{"one-file-system",'x',0,0,"This option prevents directory traversal from descending into directories that have a different device number than the file from which the descent began.",0 },
	{"start-device-only",'o',0,OPTION_ALIAS | OPTION_HIDDEN,0,0}, // This legacy can be removed in 2036 (10-year Long-Term Support)
	{"force",'f',0,0,"Use this option only in case when the PATHs that were written into the database as a result of the last scanning really need to be renewed. Warning! If this option will be used in incorrect way, information about files and their checksums against the database would be lost.",0},
	{"update",'u',0,0,"Updates the database to reflect file system changes (new, modified and deleted files). Must be used with the same initial PATH that was used when creating the database, as existing records will be replaced with data from the specified location. This option modifies database consistency. Use with caution, especially in automated scripts, as incorrect usage may lead to loss of file checksums and metadata.",0 },
	{"database",'d',"FILE",0,"Database filename. Defaults to ${HOST}.db, where HOST is the local hostname.",0 },
	{"path",'P',"PATH",0,"Filesystem root to scan. May be repeated and combined with positional PATH arguments.",0},
	{"check-level",'l',"full|quick",0,"Select database validation level: 'quick' for basic structure check, 'full' (default) for comprehensive integrity verification.",0 },
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
	{"output",'O',"full|compact|machine",0,"Output formats include full for detailed output, compact for concise automated reports, and machine for output designed to be parsed by scripts. The default is full",-1},
	{"progress",'p',0,0,"Enabling this option displays progress information but requires an initial count of files and the space they occupy to estimate execution time. The program first traverses all specified directories, counting files, folders, and symlinks before proceeding with file analysis. This initial traversal may take a significant amount of time. It is strongly recommended not to use this option when calling the program from a script.",0 },
	{"color",'S',"auto|always|never",0,"Choose when to apply color and text styling (-S means style). Use auto for terminal output, always to preserve styling in files and pipes, or never to disable it. The default is auto",-1},
	{"config",'g',"FILE",0,"Read settings from FILE. Help, usage, and version requests ignore FILE. Otherwise, cannot be combined with other CLI arguments. With no CLI arguments, read " APP_NAME ".conf in the current directory if present",-1},
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
 * @brief Report an option error with its configuration line when present
 * @param state Parser or diagnostic state supplying the program name and error stream
 * @param error_number Error number to append as system text, or zero
 * @param format Printf-style diagnostic format without the ERROR: prefix
 * @param ... Values referenced by the diagnostic format
 */
__attribute__((format(printf,3,4)))
static void report_option_error(
	const struct argp_state *state,
	int                     error_number,
	const char              *format,
	...)
{
	char *message = NULL;
	va_list args;
	va_start(args,format);
	int message_length = vasprintf(&message,format,args);
	va_end(args);

	const char *message_text = "Cannot format option diagnostic";

	if(message_length >= 0)
	{
		message_text = message;
	} else {
		error_number = errno;
	}

	if(config->config_file_error_line > 0)
	{
		/* File errors retain the same program name and stream as CLI errors */
		argp_failure(state,0,error_number,"ERROR: Line %d in config file: %s",config->config_file_error_line,message_text);
	} else {
		argp_failure(state,0,error_number,"ERROR: %s",message_text);
	}

	if(message_length >= 0)
	{
		free(message);
	}
}

/**
 * @brief Apply a named option or handle an argp parser event
 * @param key Current option key or argp event key
 * @param arg Optional option value; file flags use NULL for true and "false" for false
 * @param state CLI state with ArgumentParseContext in input, or a diagnostic state for file options
 * @return EX_OK on success, EX_USAGE for invalid usage, or errno-style error code
 */
error_t parse_opt(
	int               key,
	char              *arg,
	struct argp_state *state)
{
	/* Points to the unparsed suffix after strtol() reads the number */
	char *number_suffix = NULL;

	/* Parsed --maxdepth value before range validation and conversion to short int */
	long int argument_value = -1;

	/* Both CLI options and configuration records reach this handler.
	   config_file_error_line is -1 for CLI calls. While applying a file entry,
	   read_config_entry() sets it to the current line for error reporting and
	   resets it afterward. A positive line also identifies a file value here.

	   Before calling parse_opt(), read_config_entry() recognizes boolean
	   spellings without regard to letter case and converts them as follows:
	     true / yes / on / y / 1  -> arg is NULL, just like a CLI flag;
	     false / no / off / n / 0 -> arg points to the normalized string "false".
	   Thus, "0", "no", and the other false spellings never reach this comparison.

	   The || conditions are evaluated from left to right:
	     1. A CLI call enables the flag regardless of arg. Optional arguments,
	        such as --dry-run=MODE, are still validated in their switch branch.
	     2. A configuration call with arg == NULL enables the flag.
	     3. Otherwise, the normalized string "false" disables the flag.
	   Short-circuit evaluation prevents strcasecmp() from receiving a NULL pointer.

	   dry-run also accepts the named mode "with-checksums", which reaches this
	   handler unchanged and enables the flag. Its branch validates the mode
	   and updates the checksum setting. In CLI, --dry-run=false still reaches
	   that validation and is rejected as an unsupported mode.

	   Flag branches below assign this boolean to Config fields, so a later
	   false record can turn off a previously enabled flag. Options with ordinary
	   string or numeric arguments do not use this value */
	const bool value = config->config_file_error_line < 0 || arg == NULL || strcasecmp(arg,"false") != 0;

	/* Reject argp-marked -- before accepting operands or checking required paths.
	   Error and cleanup events must not repeat the diagnostic */
	if(state->quoted != 0 && (key == ARGP_KEY_ARG || key == ARGP_KEY_NO_ARGS || key == ARGP_KEY_END))
	{
		argp_failure(state,0,0,"ERROR: Unsupported use of --");
		return(EX_USAGE);
	}

	switch(key)
	{
		case 'd':
		{
			// Store full path to the DB file
			if(CRITICAL & m_copy_string(conf(db_primary_file_path),arg))
			{
				report_option_error(state,ENOMEM,"Memory allocation for db_file_path failed");
				return(ENOMEM);
			}

			// Store only the DB file basename
			char *tmp = strdup(arg);

			if(tmp == NULL)
			{
				(void)m_del(conf(db_primary_file_path));
				report_option_error(state,ENOMEM,"Memory allocation for db_file_name failed");
				return(ENOMEM);
			}

			const char *db_file_basename = basename(tmp);

			if(db_file_basename == NULL)
			{
				free(tmp);
				(void)m_del(conf(db_primary_file_path));
				report_option_error(state,0,"Failed to determine database base name from path '%s'",arg);
				return(EINVAL);
			}

			if(CRITICAL & m_copy_string(conf(db_file_name),db_file_basename))
			{
				free(tmp);
				(void)m_del(conf(db_primary_file_path));
				report_option_error(state,ENOMEM,"Memory allocation for db_file_name failed");
				return(ENOMEM);
			}

			free(tmp);
			break;
		}
		case 'e':
			(void)add_string_to_array(&config->ignore,arg);
			break;
		case 'n':
			config->dry_run = value;

			/* Each file value selects the complete dry-run mode */
			if(config->config_file_error_line > 0)
			{
				config->dry_run_with_checksums = false;
			}

			if(value == true && arg != NULL)
			{
				if(0 == strcasecmp(arg,"with-checksums"))
				{
					config->dry_run_with_checksums = true;

				} else {
					report_option_error(state,0,"Unsupported --dry-run mode '%s'. Supported mode: with-checksums. See --help for more information",arg);
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
			config->rehash_locked = value;
			break;
		case 'c':
			config->compare = value;
			break;
		case 'F':
			if(arg != NULL && 0 == strcasecmp(arg,"checksum-mismatch"))
			{
				config->compare_filter |= CF_CHECKSUM_MISMATCH;
			} else if(arg != NULL && 0 == strcasecmp(arg,"first-source")){
				config->compare_filter |= CF_FIRST_SOURCE;
			} else if(arg != NULL && 0 == strcasecmp(arg,"second-source")){
				config->compare_filter |= CF_SECOND_SOURCE;
			} else {
				report_option_error(state,0,"Unsupported --compare-filter value '%s'. Supported values: checksum-mismatch, first-source, second-source. See --help for more information",arg);
				return(EINVAL);
			}
			break;
		case 'o': // This legacy can be removed in 2036 (10-year Long-Term Support)
		case 'x':
			config->one_file_system = value;
			break;
		case 'C':
			config->db_drop_ignored = value;
			break;
		case 'X':
			config->db_drop_inaccessible = value;
			break;
		case 'm':
			argument_value = strtol(arg,&number_suffix,10);

			// Accept only non-negative integer values that fit into short int
			// and reject strings with trailing non-numeric characters
			if(number_suffix != arg && argument_value >= 0 && argument_value <= 32767 && *number_suffix == '\0')
			{
				config->maxdepth = (short int)argument_value;
			} else {
				report_option_error(state,0,"Wrong --maxdepth (-m) value. Should be an integer from 0 to 32767. See --help for more information");
				return(EINVAL);
			}
			break;
		case 'p':
			config->progress = value;
			/*
			 * Progress output can push important warning lines out of the visible terminal area.
			 * Set final replay together with progress before traversal starts so warnings can be shown again near exit
			 */
			config->show_remembered_messages_at_exit = value;
			break;
		case 'T':
			config->watch_timestamps = value;
			break;
		case 'u':
			config->update = value;
			break;
		case 'f':
			config->force = value;
			break;
		case 'l':
			if(0 == strcasecmp(arg,"QUICK"))
			{
				config->db_check_level = QUICK;
			} else if(0 == strcasecmp(arg,"FULL")){
				config->db_check_level = FULL;
			} else {
				report_option_error(state,0,"Unsupported --check-level value '%s'. Supported values: FULL or QUICK",arg);
				return(EINVAL);
			}
			break;
		case 's':
			config->silent = value;
			(void)update_logger_mode();
			break;
		case 'q':
			config->quiet_ignored = value;
			break;
		case 'v':
			config->verbose = value;
			(void)update_logger_mode();
			break;
		case 'O':
			if(strcasecmp(arg,"full") == 0)
			{
				config->output = OUTPUT_FULL;

			} else if(strcasecmp(arg,"compact") == 0){

				config->output = OUTPUT_COMPACT;

			} else if(strcasecmp(arg,"machine") == 0){

				config->output = OUTPUT_MACHINE;

			} else {

				report_option_error(state,0,"Unsupported --output value '%s'. Supported values: full, compact, or machine",arg);
				return(EINVAL);
			}

			(void)update_logger_mode();
			break;
		case 'S':
			if(0 == strcasecmp(arg,"auto"))
			{
				config->color_mode = COLOR_MODE_AUTO;

			} else if(0 == strcasecmp(arg,"always")){

				config->color_mode = COLOR_MODE_ALWAYS;

			} else if(0 == strcasecmp(arg,"never")){

				config->color_mode = COLOR_MODE_NEVER;

			} else {
				report_option_error(state,0,"Unsupported --color value '%s'. Supported values: auto, always, or never",arg);
				return(EINVAL);
			}

			config->color_specified = true;

			(void)update_logger_mode();
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
		case 'g':
			/* Source selection and file reading precede this main pass */
			break;
		case ARGP_KEY_NO_ARGS:
			/* Help, usage, and version requests do not require paths */
			if(information_mode_requested == true)
			{
				break;
			}

			/* Argp sends this event after parsing options if no positional
			   CLI arguments were processed. For example, --path=/mnt/data stores
			   a root in Config.roots without supplying a positional argument.
			   Configuration entries can also supply paths before this pass.
			   Continue when paths for the selected mode are already present;
			   ARGP_KEY_END checks that comparison has exactly two database paths.
			   Scan roots cannot satisfy the comparison path requirement */
			if((config->compare == false && config->roots.length != 0) ||
			        (config->compare == true && config->db_file_paths.length != 0))
			{
				break;
			}

			/* A bare invocation displays usage only when the optional file has
			   no settings. A false flag still counts as a configuration entry */
			if(state->argc == 1 && ((const ArgumentParseContext *)state->input)->config_has_entries == false)
			{
				information_mode_requested = true;
				argp_usage(state);
				break;
			}

			/* Options were supplied, but the paths required for this mode are missing */
			if(config->compare == false)
			{
				/* Explain how to supply scan roots in the selected settings source */
				const char *path_hint = "Specify a directory as a positional argument or with --path=PATH";

				if(config->config_file_path != NULL)
				{
					path_hint = "Add 'path = /path/to/directory' to the configuration file";
				}
				argp_failure(state,0,0,"ERROR: At least one scan path is required. %s",path_hint);
			}
			argp_usage(state);
			return(EX_USAGE);
			break;
		case 'P':
		case ARGP_KEY_ARG:
		{
			/* Status returned by the path append operation.
			   Assigned in either branch below before the shared result check */
			Return status;

			/*
			 * This branch receives a --path value or one positional argument.
			 * Both cases share the empty-path check and append to Config.roots
			 * when handling scan roots.
			 * Argp normally processes options before positional arguments, so
			 * `--compare` has selected the intended mode when a positional
			 * argument arrives.
			 * The configuration reader also uses ARGP_KEY_ARG for each compare
			 * entry after enabling comparison and rejecting an empty path.
			 *
			 * In compare mode, positional arguments are database file paths.
			 * Named paths and positional arguments in scan mode are filesystem
			 * roots. Named roots are cleared before comparison
			 */
			if(key == ARGP_KEY_ARG && config->compare == true)
			{
				status = m_string_array_append(conf(db_file_paths),char,arg);
			} else {
				if(arg[0] == '\0')
				{
					report_option_error(state,0,"Filesystem root path must not be empty. See --help for more information");
					return(EINVAL);
				}

				status = m_string_array_append(conf(roots),char,arg);
			}

			if((TRIUMPH & status) == 0)
			{
				/* Paths can come from CLI or a configuration file. The shared error
				   handler includes the line number for paths supplied by the file */
				if(key == 'P')
				{
					report_option_error(state,ENOMEM,"Memory allocation for --path failed");
				} else {
					report_option_error(state,ENOMEM,"Memory allocation for positional argument failed");
				}
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
					argp_failure(state,0,0,"ERROR: Too few arguments. --compare require two arguments with paths to database files. See --help for more information");
					return(EX_USAGE);
				} else if(config->db_file_paths.length > 2){
					argp_failure(state,0,0,"ERROR: Too many arguments. --compare require just two arguments with paths to database files. See --help for more information");
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

/**
 * @brief Select and parse one settings source, then finalize configuration state
 *
 * In normal scan mode, --path values and positional arguments become roots. In
 * `--compare` mode, the two positional arguments become database paths and
 * their file names are used in user-facing messages
 *
 * init_config() must initialize the configuration and global output settings
 * before this function applies settings. A preliminary argp pass selects a
 * file or CLI-only parsing. File settings are applied before the main argp
 * pass performs the shared final checks; file and CLI settings are never mixed
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

	/* Return errors without exiting and leave informational options
	   to the application's handlers */
	unsigned int parse_flags = ARGP_NO_EXIT | ARGP_NO_HELP;

	/* Reset informational-mode and source-selection state for this invocation */
	information_mode_requested = false;
	config->config_file_path = NULL;

	/* Local source-selection state shared by both argp passes and file reading.
	   The preliminary pass stores the program name and error stream in its
	   embedded diagnostic_state for diagnostics between passes */
	ArgumentParseContext context = {0};

	/*
	 * Stage 1: Preliminary CLI parsing and source selection
	 *
	 * Identify the config path, other arguments, and information requests to
	 * choose a settings source. Apply the CLI logger mode and color for early output;
	 * leave other working options to the main pass
	 */

	/* Copy the parser descriptor; the option table and help text remain shared.
	   Changing the callback below affects only this copy, so the main parser
	   keeps parse_opt() as its handler */
	struct argp preliminary_argp = argp;

	/* Collect the config path and record other arguments and information requests.
	   This callback does not apply working options such as --progress or --database */
	preliminary_argp.parser = parse_source_option;

	/* Scan CLI arguments without printing diagnostics or exiting on errors.
	   The callback updates context, config->config_file_path, logger mode, and color.
	   The return code lets source selection discard incomplete source-selection
	   results; the main pass reports CLI errors and handles help, usage, and version output */
	error_t preliminary_error = argp_parse(&preliminary_argp,argc,argv,ARGP_SILENT,0,&context);

	/* A conflict stops before reading. Information requests and preliminary
	   syntax errors select no file; output stays in the main CLI parser */
	run(select_settings_source(argc,preliminary_error,&context));

	/*
	 * Stage 2: Configuration file parsing
	 *
	 * Read a file only when source selection succeeded and selected a path.
	 * config_required makes a missing explicitly selected file an error, while
	 * a missing default file is allowed. Reading applies settings to Config and
	 * records recognized entries in config_has_entries, including false flags,
	 * so the main pass can distinguish an empty file from incomplete settings
	 */

	if(SUCCESS == status && config->config_file_path != NULL)
	{
		run(read_config_file(config->config_file_path,context.config_required,&context.config_has_entries,&context.diagnostic_state));
	}

	/* Print program identity before the main CLI pass using the logger mode
	   selected by CLI or file settings. Silent mode suppresses the message.
	   Source-selection and file errors may precede this output */
	about();

	if(CRITICAL & status)
	{
		provide(status);
	}

	/*
	 * Stage 3: Main CLI parsing and final validation
	 *
	 * Route parser events through parse_opt() to apply CLI settings when CLI
	 * is the selected source, or finalize settings already read from the file.
	 * Report CLI errors and handle help, usage, and version requests here.
	 * File settings and CLI working options are never combined
	 */

	error_t parse_error = argp_parse(&argp,argc,argv,parse_flags,0,&context);

	/* Even if a transient preliminary error does not recur, it must prevent
	   the operation from starting after source selection was abandoned */
	if(parse_error == EX_OK && preliminary_error != EX_OK)
	{
		parse_error = preliminary_error;
	}

	if(parse_error != EX_OK)
	{
		status = FAILURE;
	} else if(information_mode_requested == true){
		provide(INFO | HALTED);
	}

	/* Discard scan roots before comparison path handling and diagnostics */
	if((SUCCESS & status) && config->compare == true)
	{
		run(m_array_del(conf(roots)));
	}

	/*
	 * Normal scan mode stores named and positional paths as traversal roots.
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

#ifdef TESTITALL_TEST_HOOKS
	/* Show key-value diagnostics consumed by test templates */
	show_testing_diagnostics();
#endif

	/* Show compact configuration summary for verbose user output */
	show_verbose_diagnostics();

	slog(TRACE,"Argument parsing is finished\n");

	provide(status);
}
