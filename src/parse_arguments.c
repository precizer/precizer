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
"\033[1mprecizer\033[m is a CLI application designed to check the integrity of files after synchronization. The program recursively traverses directories and creates a database of files and their checksums, followed by a quick comparison.\n\
\n\
\033[1mprecizer\033[m is focused on work with gigantic file systems. With the program it is possible to find synchronization errors by comparing data with files and their checksums from different sources. Or it can be used to crawling historical changes by comparing databases from the same sources over different times.\n\
\nGlory to Ukraine!\n" \
"\vSIMPLE EXAMPLE\n\
Assuming there are two hosts with large disks and identical contents mounted in /mnt1 and /mnt2 accordingly. The general task is to check whether the content is absolutely identical or whether there are differences.\n" \
"Run the program on the first machine with host name, for example “host1”:\n\
\n\
precizer --progress /mnt1\n\
\n\
As a result of the program running all directories starting from /mnt1 will be recursively traversed and the host1.db database will be created in the current directory. The --progress option visualizes progress and will show the amount of space and the number of files being examined.\n\
\n\
Run the program on a second machine with a host name, for example host2:\n\
\n\
precizer --progress /mnt2\n\
\n\
As a result, the host2.db database will be created in the current directory.\n\
\n\
Copy the files with the host1.db and host2.db databases to one of the machines and run the program with the appropriate parameters to compare the databases:\n\
\n\
precizer --compare host1.db host2.db\n\
\n\
Note that precizer only writes relative paths to the database. The example file /mnt1/abc/def/aaa.txt will be written to the database as \"abc/def/aaa.txt\" without /mnt1. The same thing will happen with the file /mnt2/abc/def/aaa.txt. Despite different mount points and different sources the files can be compared with each other under the same names \"abc/def/aaa.txt\" with the corresponding checksums.\n\
\n\
As a result of the program running, the following information will be displayed on the screen:\n\
* Which files are missing on host1 but present on host2 and vice versa.\n\
* For which files, present on both hosts, the checksums do NOT match.\n\
\n" \
"All other technical details could be found in README file of the project";

/* A description of the arguments we accept. */
static char args_doc[] = "PATH";

/* The options we understand. */
static struct argp_option options[] = {
	{ 0, 0, 0, 0, "Build database options:", 2},
	{"ignore",   'e', "PCRE2_REGEXP", 0, "Relative path to ignore. PCRE2 regular expressions " \
	                                     "could be used to specify a pattern to ignore files " \
	                                     "or directories. Attention! All paths for the regular " \
	                                     "expression must be  specified as relative. To " \
	                                     "understand what a relative path looks like, just " \
	                                     "run traverses without the \033[1m--ignore\033[0m option " \
	                                     "and look how the terminal will display relative paths " \
	                                     "that are written to the database.\n" \
	                                     "\nExamples:\n" \
	                                     "\n\033[1m--ignore=\"diff2/1/*\" tests/examples/diffs\033[0m\n" \
	                                     "\n" \
	                                     "In this example, the starting path for the traversing " \
	                                     "is ./tests/examples/diffs and the relative path to ignore will " \
	                                     "be ./tests/examples/diffs/diff2/1/ and all subdirectories (/*).\n" \
	                                     "\n" \
	                                     "Multiple regular expressions for ignore could be specified using " \
	                                     "many \033[1m--ignore\033[0m options at once:\n" \
	                                     "\n" \
	                                     "\033[1m--ignore=\"diff2/1/*\" --ignore=\"diff2/2/*\" " \
	                                     "tests/examples/diffs\033[0m\n", 0 },
	{"include",   'i', "PCRE2_REGEXP", 0, "Relative path to be included. PCRE2 regular expressions. " \
	                                     "Include these relative paths even if they were excluded " \
	                                     "via the \033[1m--ignore\033[0m option. Multiple regular " \
	                                     "expressions could be specified\n", 0 },
	{"db-clean-ignored",   'C', 0, 0,    "The database is protected from accidental changes by default. " \
	                                     "The option \033[1m--db-clean-ignored\033[0m must be specifyed additionally " \
	                                     "in order to remove from the database mention of files that " \
	                                     "matches the regular expression passed through the " \
	                                     "\033[1m--ignore=PCRE2_REGEXP\033[0m option(s)\n", 0},
	{"maxdepth", 'm', "NUMBER", 0, "Recursion depth limit. " \
	                        "The depth of the traversal, numbered from 0 to N, " \
	                        "where a file could be found. Representing the maximum " \
	                        "of the starting point (from root) of the traversal. " \
	                        "The root itself is numbered 0\n" \
	                        "\033[1m--maxdepth=0\033[0m completely disable recursion\n", 0 },
	{"force",    'f', 0, 0, "Use this option only in case when the PATHs that were written into " \
	                        "the database as a result of the last scaning really need to be " \
	                        "renewed. Warning! If this option will be used in incorrect way, " \
	                        "information about files and their checksums against the database would " \
	                        "be lost.\n", 0 },
	{"update",   'u', 0, 0, "Force update of the database with new, changed and deleted files. "\
	                        "This option would greatly affects the security of the database. " \
	                        "Please use it only in exceptional cases and not add thoughtlessly " \
	                        "to a script. If use this parameter in incorrect way information against " \
	                        "the database containing file names and their checksums may be lost. " \
	                         "When this argument is specified, it is important to specify the same " \
	                         "PATH source (starting directory) that has been specified when the database " \
	                         "had been created. Otherwise, updating the data makes no sense — the " \
	                         "old data will be deleted from the database and completely " \
	                         "overwritten by new ones.\n", 0 },
	{"database", 'd', "FILE", 0, "Database file name. By default name of the local host will be used: ${HOST}.db\n", 0 },
	{ 0, 0, 0, 0, "Compare databases options:", 1},
	{"compare",  'c', 0, 0, "Compare two databases from different sourses. Two extra arguments should be " \
	                        "specified as paths to the databases files to compare. For example: \033[1m--compare database1.db database2.db\033[0m\n", 0 },
	{ 0, 0, 0, 0, "Visualizations options:\n", -1},
	{"silent",   's', 0, 0, "Don't produce any output. Option cannot be used with \033[1m--compare\033[0m", 0 },
	{"verbose",  'v', 0, 0, "Produce verbose output.", 0 },
	{"progress", 'p', 0, 0, "Show progress bar. This assume a preliminary count of files and the space they occupy " \
	                        "to predict execution time. It is strongly recommended not to specify this option " \
	                        "if the program is called from a script. This will reduce execution time " \
	                        "(sometimes significantly) and reduce screen output.", 0 },
	{0}
};

/* Used to communicate with parse_opt. */
struct arguments
{
	char **filenames;    /* part of path */
};

/* Parse a single option. */
static error_t parse_opt
(
	int key,
	char *arg,
	struct argp_state *state
)
{
	/* Get the input argument from argp_parse, which we
	know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;
	char *ptr = NULL;
	long int argument_value = -1;

	switch (key)
	{
		case 'd':
			config->db_file_name = (char *)calloc(strlen(arg) + 1,sizeof(char));
			if(config->db_file_name == NULL)
			{
				argp_failure(state, 1, 0, "ERROR: Memory allocation did not complete successfully!");
				exit(ARGP_ERR_UNKNOWN);
			}
			strcpy(config->db_file_name,arg);
			break;
		case 'e':
			add_string_to_array(&config->ignore,arg);
			break;
		case 'i':
			add_string_to_array(&config->include,arg);
			break;
		case 'c':
			config->compare = true;
			break;
		case 'C':
			config->db_clean_ignored = true;
			break;
		case 'm':
			argument_value = strtol(arg, &ptr, 10);
			// Check up if lont int could be casted to short int
			// and the argument contains a digit only
			if(argument_value >= 0 && argument_value <= 32767 && *ptr == '\0')
			{
				config->maxdepth = (short int)argument_value;
			} else {
				argp_failure(state, 1, 0, "ERROR: Wrong --maxdepth (-m) value. Should be an integer from 0 to 32767. See --help for more information");
			}
			break;
		case 'p':
			config->progress = true;
			break;
		case 'u':
			config->update = true;
			break;
		case 'f':
			config->force = true;
			break;
		case 's':
			config->silent = true;
			break;
		case 'v':
			config->verbose = true;
			break;
		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;
		case ARGP_KEY_ARG:
			config->paths = &state->argv[state->next - 1];
			arguments->filenames = config->paths;
			state->next = state->argc;
			break;
		case ARGP_KEY_END:
			if(config->compare == true)
			{
				if(state->arg_num < 2)
				{
					argp_failure(state, 1, 0, "ERROR: Too few arguments\n--compare require two arguments with paths to database files. See --help for more information");
				} else if (state->arg_num > 2)
				{
					argp_failure(state, 1, 0, "ERROR: Too many arguments\n--compare require just two arguments with paths to database files. See --help for more information");
				}
			} else {
				if(state->arg_num > 1)
				{
					argp_failure(state, 1, 0, "ERROR: Too many arguments\nOnly one PATH argument can be used for traversing file hierarchy. See --help for more information");
				}
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

Return parse_arguments
(
	const int argc,
	char * argv[]
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	struct arguments arguments;

	/* Default values to 0. */
	memset(&arguments,0,sizeof(struct arguments));

	/* Parse our arguments; every option seen by parse_opt will be
	reflected in arguments. */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if(config->compare == true)
	{
		// The array with database names
		config->databases_to_compare = config->paths;
		for (int j = 0; config->paths[j]; j++)
		{
			// Extract file name from a path
			arguments.filenames[j] = basename(config->databases_to_compare[j]);
		}

	} else {
		for (int j = 0; config->paths[j]; j++)
		{
			// Remove unnecessary trailing slash at the end of the directory path
			remove_trailing_slash(config->paths[j]);
		}
	}

	config->filenames = arguments.filenames;

	if(config->verbose == true
		&& config->silent == false)
	{
		slog(false,"Configuration: ");
		printf("paths=");
		for (int j = 0; config->paths[j]; j++)
		{
			printf(j == 0 ? "%s" : ", %s", config->paths[j]);
		}
		printf("; ");
		printf("ignore=");
		// Print the contents of the string array
		for(int i = 0; config->ignore[i] != NULL; ++i) {
			printf(i == 0 ? "%s" : ", %s", config->ignore[i]);
		}
		printf("; ");
		printf("include=");
		// Print the contents of the string array
		for(int i = 0; config->include[i] != NULL; ++i) {
			printf(i == 0 ? "%s" : ", %s", config->include[i]);
		}
		printf("; ");
		printf("verbose=%s; silent=%s; force=%s; update=%s; progress=%s; compare=%s, db-clean-ignored=%s",
		config->verbose ? "yes" : "no",
		config->silent ? "yes" : "no",
		config->force ? "yes" : "no",
		config->update ? "yes" : "no",
		config->progress ? "yes" : "no",
		config->compare ? "yes" : "no",
		config->db_clean_ignored ? "yes" : "no");
		printf("\n");
	}

	return(status);
}
