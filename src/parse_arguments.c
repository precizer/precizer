#include "precizer.h"
#include <argp.h>

/**
 *
 * @brief Parse arguments with argp lib
 *
 */

const char *argp_program_version = APP_NAME " " APP_VERSION;

/* Program documentation. */
static char doc[] =
        BOLD "precizer" RESET " is a CLI application for verifying file integrity after synchronization. The program recursively traverses directories, creates a database of files with their checksums, and performs comparisons.\n" \
        "\n" \
        BOLD "precizer" RESET " is optimized for large-scale file systems. It detects synchronization errors by cross-referencing data and checksums from different sources. It can also track historical changes by comparing databases from the same sources across different time periods.\n" \
        "\n" \
        "Glory to Ukraine!\n" \
        "\vSIMPLE EXAMPLE\n" \
        "\n" \
        "Consider two hosts with large disks containing identical content mounted at /mnt1 and /mnt2 respectively. The task is to verify content identity and identify any differences.\n" \
        "\n" \
        "1. Run the program on the first machine with host name, for example “host1”:\n" \
        "\n" \
        "precizer --progress /mnt1\n" \
        "\n" \
        "The program recursively traverses all directories starting from /mnt1 and the host1.db database will be created in the current directory. The --progress option visualizes progress and will show the amount of space and the number of files being examined.\n" \
        "\n" \
        "2. Run the program on a second machine with a host name, for example host2:\n" \
        "\n" \
        "precizer --progress /mnt2\n" \
        "\n" \
        "As a result, the host2.db database will be created in the current directory.\n" \
        "\n" \
        "3. Transfer the host1.db and host2.db files to either machine and run the program with the appropriate parameters to compare the databases:\n" \
        "\n" \
        "precizer --compare host1.db host2.db\n" \
        "\n" \
        "The following information will be displayed on the screen:\n" \
        "\n" \
        "* Which files are missing on “host1” but present on “host2” and vice versa.\n" \
        "* For which files, present on both hosts, the checksums do NOT match.\n" \
        "\n" \
        "Note that precizer writes only relative paths to the database. The example file “/mnt1/abc/def/aaa.txt” will be written to the database as “abc/def/aaa.txt” without /mnt1. The same thing will happen with the file “/mnt2/abc/def/aaa.txt”. Despite different mount points and different sources the files can be compared with each other under the same names “abc/def/aaa.txt” with the corresponding checksums.\n" \
        "\n" \
        "All other technical details could be found in README file of the project";

/* A description of the arguments we accept. */
static char args_doc[] = "PATH";

/* The options we understand. */
static struct argp_option options[] = {
	{ 0,0,0,0,"Build database options:",2},
	{"start-device-only",'o',0,0,"This option prevents directory traversal from descending into directories " \
	 "that have a different device number than the file from  " \
	 "which the descent began\n",0 },
	{"ignore",'e',"PCRE2_REGEXP",0,"Relative path to ignore. PCRE2 regular expressions " \
	 "could be used to specify a pattern to ignore files " \
	 "or directories. Attention! All paths for the regular " \
	 "expression must be  specified as relative. To " \
	 "understand what a relative path looks like, just " \
	 "run traverses without the " BOLD "--ignore" RESET " option " \
	 "and look how the terminal will display relative paths " \
	 "that are written to the database.\n" \
	 "\nExamples:\n" \
	 "\n" BOLD "--ignore=\"diff2/1/*\" tests/examples/diffs" RESET "\n" \
	 "\n" \
	 "In this example, the starting path for the traversing " \
	 "is ./tests/examples/diffs and the relative path to ignore will " \
	 "be ./tests/examples/diffs/diff2/1/ and all subdirectories (/*).\n" \
	 "\n" \
	 "Multiple regular expressions for ignore could be specified using " \
	 "many " BOLD "--ignore" RESET " options at once:\n" \
	 "\n" \
	 BOLD "--ignore=\"diff2/1/*\" --ignore=\"diff2/2/*\" " \
	 "tests/examples/diffs" RESET "\n",0 },
	{"include",'i',"PCRE2_REGEXP",0,"Relative path to be included. PCRE2 regular expressions. " \
	 "Include these relative paths even if they were excluded " \
	 "via the " BOLD "--ignore" RESET " option. Multiple regular " \
	 "expressions could be specified\n",0 },
	{"db-clean-ignored",'C',0,0,"The database is protected from accidental changes by default. " \
	 "The option " BOLD "--db-clean-ignored" RESET " must be specified additionally " \
	 "in order to remove from the database mention of files that " \
	 "matches the regular expression passed through the " \
	 BOLD "--ignore=PCRE2_REGEXP" RESET " option(s)\n",0},
	{"dry-run",'n',0,0,"Perform a trial run with no changes made. The option will not affect " BOLD "--compare" RESET "\n",0},
	{"watch-timestamps",'T',0,0,"Consider file metadata changes (creation and modification timestamps) " \
	 "in addition to file size when detecting changes. By default, only " \
	 "file size changes trigger rescanning. When this option is enabled, " \
	 "any changes to file timestamps or size will cause the file to be " \
	 "rescanned and its checksum updated in the primary database\n",0},
	{"maxdepth",'m',"NUMBER",0,"Recursion depth limit. " \
	 "The depth of the traversal, numbered from 0 to N, " \
	 "where a file could be found. Representing the maximum " \
	 "of the starting point (from root) of the traversal. " \
	 "The root itself is numbered 0\n" \
	 BOLD "--maxdepth=0" RESET " completely disable recursion\n",0 },
	{"force",'f',0,0,"Use this option only in case when the PATHs that were written into " \
	 "the database as a result of the last scanning really need to be " \
	 "renewed. Warning! If this option will be used in incorrect way, " \
	 "information about files and their checksums against the database would " \
	 "be lost.\n",0 },
	{"update",'u',0,0,"Updates the database to reflect file system changes (new, " \
	 "modified and deleted files). Must be used with the same " \
	 "initial PATH that was used when creating the database, as " \
	 "existing records will be replaced with data from the " \
	 "specified location.\n" \
	 "This option modifies database consistency. Use with caution, " \
	 "especially in automated scripts, as incorrect usage may lead " \
	 "to loss of file checksums and metadata.\n",0 },
	{"database",'d',"FILE",0,"Database filename. Defaults to ${HOST}.db, where HOST is the local hostname\n",0 },
	{"check-level",'l',"FULL|QUICK",0,"Select database validation level: 'quick' for basic structure check, 'full' (default) for comprehensive integrity verification\n",0 },
	{ 0,0,0,0,"Compare databases options:",1},
	{"compare",'c',0,0,"Compare two databases from different sources. Requires two additional arguments " \
	 "specifying paths to database files, e.g.:\n" BOLD " --compare database1.db database2.db" RESET "\n",0 },
	{ 0,0,0,0,"Visualizations options:\n",-1},
	{"silent",'s',0,0,"Don't produce any output. The option will not affect " BOLD "--compare" RESET,0 },
	{"verbose",'v',0,0,"Produce verbose output.",0 },
	{"progress",'p',0,0,"Show progress bar. This assume a preliminary count of files and the space they occupy " \
	 "to predict execution time. It is strongly recommended not to specify this option " \
	 "if the program is called from a script. This will reduce execution time " \
	 "(sometimes significantly) and reduce screen output.",0 },
	{0}
};

/* Parse a single option. */
static error_t parse_opt(
	int               key,
	char              *arg,
	struct argp_state *state
){
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	//struct arguments *arguments = state->input;
	char *ptr = NULL;
	long int argument_value = -1;

	switch(key)
	{
		case 'd':
			// Full path to DB file
			config->db_file_path = strdup(arg);

			if(config->db_file_path == NULL)
			{
				argp_failure(state,1,0,"ERROR: Memory allocation for db_file_path failed!");
				exit(ARGP_ERR_UNKNOWN);
			}

			// Name of DB file only
			config->db_file_name = strdup(basename(arg));

			if(config->db_file_name == NULL)
			{
				argp_failure(state,1,0,"ERROR: Memory allocation for db_file_name failed!");
				exit(ARGP_ERR_UNKNOWN);
			}
			break;
		case 'e':
			(void)add_string_to_array(&config->ignore,arg);
			break;
		case 'n':
			config->dry_run = true;
			break;
		case 'i':
			(void)add_string_to_array(&config->include,arg);
			break;
		case 'c':
			config->compare = true;
			break;
		case 'o':
			config->start_device_only = true;
			break;
		case 'C':
			config->db_clean_ignored = true;
			break;
		case 'm':
			argument_value = strtol(arg,&ptr,10);

			// Validate if lont int could be casted to short int
			// and the argument contains a digit only
			if(argument_value >= 0 && argument_value <= 32767 && *ptr == '\0')
			{
				config->maxdepth = (short int)argument_value;
			} else {
				argp_failure(state,1,0,"ERROR: Wrong --maxdepth (-m) value. Should be an integer from 0 to 32767. See --help for more information");
			}
			break;
		case 'p':
			config->progress = true;
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

			if(0 == strncasecmp(arg,"QUICK",sizeof("QUICK")))
			{
				config->db_check_level = QUICK;
			} else if(0 == strncasecmp(arg,"FULL",sizeof("FULL"))){
				config->db_check_level = FULL;
			} else {
				return ARGP_ERR_UNKNOWN;
			}
			break;
		case 's':
			// Global variable
			rational_logger_mode = SILENT;
			break;
		case 'v':
			// Global variable
			rational_logger_mode = VERBOSE;
			config->verbose = true;
			break;
		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;
		case ARGP_KEY_ARG:
			config->paths = &state->argv[state->next - 1];
			state->next = state->argc;
			break;
		case ARGP_KEY_END:

			if(config->compare == true)
			{
#if 0

				if(config->update == true)
				{
					argp_failure(state,1,0,"ERROR: Using arguments --compare and --update together makes no sense");

				} else
#endif

				if(state->arg_num < 2)
				{
					argp_failure(state,1,0,"ERROR: Too few arguments\n--compare require two arguments with paths to database files. See --help for more information");
				} else if(state->arg_num > 2){
					argp_failure(state,1,0,"ERROR: Too many arguments\n--compare require just two arguments with paths to database files. See --help for more information");
				}
			} else {
				if(state->arg_num > 1)
				{
					argp_failure(state,1,0,"ERROR: Too many arguments\nOnly one PATH argument can be used for traversing file hierarchy. See --help for more information");
				}
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

/* Our argp parser. */
static struct argp argp = {
	options,parse_opt,args_doc,doc,0,0,0
};

Return parse_arguments(
	const int argc,
	char      *argv[]
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Parse our arguments; every option seen by parse_opt will be
	   reflected in arguments. */
	argp_parse(&argp,argc,argv,0,0,0);

	if(config->paths != NULL)
	{
		for(int j = 0; config->paths[j]; j++)
		{
			// Remove unnecessary trailing slash at the end of the directory path
			remove_trailing_slash(config->paths[j]);
		}
	}

	if(config->compare == true)
	{
		if(config->paths != NULL)
		{
			// The array with database names
			config->db_file_paths = config->paths;

			for(int j = 0; config->db_file_paths[j] && (SUCCESS == status); j++)
			{
				// Create a copy of the path string for basename
				char *tmp = strdup(config->db_file_paths[j]);

				if(tmp == NULL)
				{
					report("Failed to duplicate string: %s",config->db_file_paths[j]);
					status = FAILURE;
					break;
				}

				// Get basename and handle possible NULL return
				char *db_file_basename = basename(tmp);

				if(db_file_basename == NULL)
				{
					report("basename failed for path: %s",tmp);
					free(tmp);
					status = FAILURE;
					break;
				}

				status = add_string_to_array(&config->db_file_names,db_file_basename);
				free(tmp);

				if(SUCCESS != status)
				{
					break;
				}
			}
		}
	}

	if(SUCCESS != status)
	{
		return(status);
	}

	if(rational_logger_mode & TESTING)
	{
		slog(TESTING,"rational_logger_mode=%s\n",rational_reconvert(rational_logger_mode));

		if(config->paths != NULL)
		{
			slog(TESTING,"argument:paths=");

			for(int j = 0; config->paths[j]; j++)
			{
				printf(j == 0 ? "%s" : ", %s",config->paths[j]);
			}
			printf("\n");
		}

		if(config->db_file_paths != NULL)
		{
			slog(TESTING,"argument:db_file_paths=");

			for(int j = 0; config->db_file_paths[j]; j++)
			{
				printf(j == 0 ? "%s" : ", %s",config->db_file_paths[j]);
			}
			printf("\n");
		}

		if(config->db_file_names != NULL)
		{
			slog(TESTING,"argument:db_file_names=");

			for(int j = 0; config->db_file_names[j]; j++)
			{
				printf(j == 0 ? "%s" : ", %s",config->db_file_names[j]);
			}
			printf("\n");
		}

		if(config->ignore != NULL)
		{
			slog(TESTING,"argument:ignore=");

			// Print the contents of the string array
			for(int i = 0; config->ignore[i] != NULL; ++i)
			{
				printf(i == 0 ? "%s" : ", %s",config->ignore[i]);
			}
			printf("\n");
		}

		if(config->include != NULL)
		{
			slog(TESTING,"argument:include=");

			// Print the contents of the string array
			for(int i = 0; config->include[i] != NULL; ++i)
			{
				printf(i == 0 ? "%s" : ", %s",config->include[i]);
			}
			printf("\n");
		}

		if(config->db_check_level != FULL)
		{
			slog(TESTING,"argument:check-level=%s\n",config->db_check_level == QUICK ? "QUICK" : "FULL");
		}

		if(config->verbose)
		{
			slog(TESTING,"argument:verbose=%s\n",config->verbose ? "yes" : "no");
		}

		if(config->watch_timestamps)
		{
			slog(TESTING,"argument:watch-timestamps=%s\n",config->watch_timestamps ? "yes" : "no");
		}

		if(config->force)
		{
			slog(TESTING,"argument:force=%s\n",config->force ? "yes" : "no");
		}

		if(config->update)
		{
			slog(TESTING,"argument:update=%s\n",config->update ? "yes" : "no");
		}

		if(config->progress)
		{
			slog(TESTING,"argument:progress=%s\n",config->progress ? "yes" : "no");
		}

		if(config->compare)
		{
			slog(TESTING,"argument:compare=%s\n",config->compare ? "yes" : "no");
		}

		if(config->db_clean_ignored)
		{
			slog(TESTING,"argument:db-clean-ignored=%s\n",config->db_clean_ignored ? "yes" : "no");
		}

		if(config->dry_run)
		{
			slog(TESTING,"argument:dry-run=%s\n",config->dry_run ? "yes" : "no");
		}
	} else {

		// Verbose but NOT silent
		if((rational_logger_mode & VERBOSE) && !(rational_logger_mode & SILENT))
		{
			slog(VERBOSE,"Configuration: ");
			printf("rational_logger_mode=%s\n",rational_reconvert(rational_logger_mode));

			if(config->paths != NULL)
			{
				printf("paths=");

				for(int j = 0; config->paths[j]; j++)
				{
					printf(j == 0 ? "%s" : ", %s",config->paths[j]);
				}
				printf("; ");
			}

			if(config->db_file_paths != NULL)
			{
				printf("db_file_paths=");

				for(int j = 0; config->db_file_paths[j]; j++)
				{
					printf(j == 0 ? "%s" : ", %s",config->db_file_paths[j]);
				}
				printf("; ");
			}

			if(config->db_file_names != NULL)
			{
				printf("db_file_names=");

				for(int j = 0; config->db_file_names[j]; j++)
				{
					printf(j == 0 ? "%s" : ", %s",config->db_file_names[j]);
				}
				printf("; ");
			}

			if(config->ignore != NULL)
			{
				printf("ignore=");

				// Print the contents of the string array
				for(int i = 0; config->ignore[i] != NULL; ++i)
				{
					printf(i == 0 ? "%s" : ", %s",config->ignore[i]);
				}
				printf("; ");
			}

			if(config->include != NULL)
			{
				printf("include=");

				// Print the contents of the string array
				for(int i = 0; config->include[i] != NULL; ++i)
				{
					printf(i == 0 ? "%s" : ", %s",config->include[i]);
				}
				printf("; ");
			}
			printf("verbose=%s; silent=no; force=%s; update=%s; watch-timestamps=%s; progress=%s; compare=%s, db-clean-ignored=%s, dry-run=%s, check-level=%s, rational_logger_mode=%s",
				config->verbose ? "yes" : "no",
				config->force ? "yes" : "no",
				config->update ? "yes" : "no",
				config->watch_timestamps ? "yes" : "no",
				config->progress ? "yes" : "no",
				config->compare ? "yes" : "no",
				config->db_clean_ignored ? "yes" : "no",
				config->dry_run ? "yes" : "no",
				config->db_check_level == QUICK ? "QUICK" : "FULL",
				rational_reconvert(rational_logger_mode));
			printf("\n");
		}
	}

	slog(TRACE,"Arguments parsed\n");

	return(status);
}
