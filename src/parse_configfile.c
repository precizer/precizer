#include "precizer.h"
#include <sysexits.h>

/* Shared CLI option table */
extern const struct argp_option options[];

/**
 * @brief Share record handling results with the file reader through inih's user pointer
 *
 * One local instance belongs to each read_config_file() call. has_entries
 * records whether a recognized, permitted key was encountered, including a
 * false flag that disables an option. status records handler failure, so the
 * reader does not add a generic syntax error after a diagnostic was already
 * reported. state supplies the program name and error stream for diagnostics.
 * The settings themselves are stored in Config
 */
typedef struct {
	bool has_entries;
	Return status;
	struct argp_state *state;
} ConfigFileReadContext;

/**
 * @brief Resolve a configuration key and apply its value through parse_opt
 *
 * Strip one matching pair of outer quotes before interpreting the value.
 * Interior spaces and backslashes are preserved after inih handles comments
 *
 * @param user Configuration read context
 * @param section Ignored section name supplied by inih
 * @param name Exact long option name, or NULL for a section header
 * @param value Parsed value, valid only during this call
 * @param line Original physical line number
 * @return Nonzero when a setting is applied or a section is ignored, or zero after an error
 */
static int read_config_entry(
	void       *user,
	const char *section __attribute__((unused)),
	const char *name,
	const char *value,
	int        line)
{
	ConfigFileReadContext *context = user;

	/* Section headers only organize the file; all keys share the same settings */
	if(name == NULL)
	{
		return(1);
	}

	/* Help, usage, version, and configuration file selection are available only
	   through CLI. Reject these keys before parsing their values, so even
	   help = false is an error. The lookup below accepts only exact long names
	   without leading dashes; short forms such as h, V, and -h are rejected
	   as unknown keys */
	if(strcmp(name,"help") == 0 || strcmp(name,"usage") == 0 ||
	        strcmp(name,"version") == 0 || strcmp(name,"config") == 0)
	{
		argp_failure(context->state,0,0,"ERROR: Line %d in config file: Config key '%s' is only available in CLI",line,name);
		context->status = FAILURE;
		return(0);
	}

	/* For an option entry, argp_option.name is the long CLI name, key selects
	   its parse_opt() branch, and arg names the expected value (NULL for a flag).
	   This pointer refers to an existing options[] entry without copying it.
	   Keep the most recent non-alias entry so an alias can use its primary
	   option's value requirements and handler key */
	const struct argp_option *option = NULL;

	/* Find this configuration key in the shared CLI table. Stop at the zero
	   terminator, not at the first entry without a name: help group headings
	   and aliases that provide only a short option also have NULL names.
	   Compare the supplied key with each long name exactly, including case.
	   On a match, convert flag values and apply the setting, then return.
	   Reaching the end without a match produces the unknown-key error below */
	for(size_t i = 0; options[i].name != NULL || options[i].key != 0 ||
	        options[i].doc != NULL || options[i].group != 0; i++)
	{
		/* Aliases inherit the argument requirements of their primary option.
		   Match the alias's own name below while retaining the primary entry */
		if((options[i].flags & OPTION_ALIAS) == 0)
		{
			option = &options[i];
		}

		if(options[i].name != NULL && strcmp(name,options[i].name) == 0)
		{
			/* A false flag still counts as an explicit setting */
			context->has_entries = true;

			/* Whether parse_opt() needs the prepared value as its argument.
			   Remains true for options with values and for false flags, which
			   receive the normalized string "false". Becomes false for true
			   flags, which receive NULL just like flags supplied through CLI */
			bool takes_value = true;

			/* Number of value bytes to copy, excluding the terminating null byte.
			   Initially includes any outer quotes; reduced by two when they are removed.
			   inih has already removed surrounding whitespace and comments.
			   Only a leading single or double quote starts a quoted value;
			   otherwise the pointer and length retain the entire value */
			size_t value_length = strlen(value);

			if(value[0] == '"' || value[0] == '\'')
			{
				/* A quoted value needs at least two characters, and its final
				   character must match the opening quote. A lone opening quote
				   or a different final character is an error on this physical line */
				if(value_length < 2 || value[value_length - 1] != value[0])
				{
					/* Report the key and line, then stop reading. Recording the failure
					   prevents the file reader from adding a generic syntax error */
					argp_failure(context->state,0,0,"ERROR: Line %d in config file: Unclosed quoted value for '%s'",line,name);

					context->status = FAILURE;

					return(0);
				}

				/* Skip the opening quote and exclude both quotes from the copy length.
				   The original text is not modified. Only this outer pair is removed;
				   inner quotes, spaces, and backslashes remain literal characters.
				   An empty pair produces a zero-length value for normal option validation */
				value++;

				value_length -= 2;
			}

			/* Argp accepts mutable arguments, so copy the selected bytes into a local
			   buffer. inih bounds the input with INI_MAX_LINE, so the value fits.
			   Add the terminating null byte explicitly: memcpy() copies only the
			   selected contents, excluding any closing quote and original terminator */
			char value_buffer[INI_MAX_LINE];

			memcpy(value_buffer,value,value_length);

			value_buffer[value_length] = '\0';

			/* Interpret booleans and other option values from this prepared copy,
			   so quoted and unquoted spellings reach the same validation logic */
			value = value_buffer;

			/* Flags accept boolean values, except compare, whose file entries
			   contain database paths. dry-run also accepts a named mode,
			   which is validated by parse_opt() when it is not a boolean */
			if((option->arg == NULL && option->key != 'c') || option->key == 'n')
			{
				/* Normalize false spellings for the shared option handler */
				if(strcasecmp(value,"false") == 0 || strcasecmp(value,"no") == 0 ||
				        strcasecmp(value,"off") == 0 || strcasecmp(value,"n") == 0 ||
				        strcasecmp(value,"0") == 0)
				{
					strcpy(value_buffer,"false");

				} else if(strcasecmp(value,"true") == 0 || strcasecmp(value,"yes") == 0 ||
				        strcasecmp(value,"on") == 0 || strcasecmp(value,"y") == 0 ||
				        strcasecmp(value,"1") == 0){
					/* A true flag reaches parse_opt() without a value, as in CLI */
					takes_value = false;

				} else if(option->key != 'n'){

					argp_failure(context->state,0,0,"ERROR: Line %d in config file: Unsupported --%s value '%s'. Supported values: true/yes/on/y/1 or false/no/off/n/0",line,name,value);
					context->status = FAILURE;
					return(0);
				}
			}

			char *argument = NULL;

			if(takes_value == true)
			{
				argument = value_buffer;
			}

			/* Each compare entry enables comparison and supplies one database
			   path through the existing positional-argument handler */
			int key = option->key;

			if(key == 'c')
			{
				if(argument[0] == '\0')
				{
					argp_failure(context->state,0,0,"ERROR: Line %d in config file: Database path for 'compare' must not be empty",line);
					context->status = FAILURE;
					return(0);
				}

				config->compare = true;
				key = ARGP_KEY_ARG;
			}

			/* Make the line available to shared option diagnostics only while this
			   entry is being applied. Reset it even when parse_opt() returns an
			   error, so later CLI processing cannot inherit a file line */
			config->config_file_error_line = line;

			error_t parse_error = parse_opt(key,argument,context->state);

			config->config_file_error_line = -1;

			if(parse_error != EX_OK)
			{
				context->status = FAILURE;
				return(0);
			}

			return(1);
		}
	}

	argp_failure(context->state,0,0,"ERROR: Line %d in config file: Unknown config key '%s'",line,name);
	context->status = FAILURE;
	return(0);
}

/**
 * @brief Read configuration with inih and apply options through parse_opt
 *
 * Config must be initialized before reading. Recognized records are applied
 * in file order regardless of section. Section headers are ignored, and
 * reading stops at the first syntax, name, or value error.
 * Required parameters and option combinations are checked during finalization
 *
 * @param path Configuration file path, relative to the working directory or absolute
 * @param required Whether a missing file is an error; other open errors always fail
 * @param has_entries Set when at least one recognized, permitted record is read
 * @param state Diagnostic state with the program name and error stream
 * @return SUCCESS on completion or an optional missing file, otherwise FAILURE
 */
Return read_config_file(
	const char        *path,
	bool              required,
	bool              *has_entries,
	struct argp_state *state)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Output flag for whether a recognized, permitted setting key was read.
	   Blank lines, comments, and section headers do not count; false flags do.
	   This distinguishes a file without settings from one that leaves defaults
	   unchanged. Start with no entries, including when an optional file is absent.
	   The return status reports errors separately; this flag does not mean success */
	*has_entries = false;

	FILE *file = fopen(path,"rb");

	if(file == NULL)
	{
		if(required || errno != ENOENT)
		{
			argp_failure(state,0,errno,"ERROR: Cannot open config file '%s'",path);
			status = FAILURE;
		}

	} else {

		/* inih reads the open file and calls read_config_entry() for each record.
		   The callback applies settings and records any error it reports.
		   Copy its status and setting-presence flag back after parsing.
		   If the callback succeeded, check for a stream read error first, then
		   a parser error, using the returned line number for syntax diagnostics.
		   The status checks prevent a second message for an already reported error */
		ConfigFileReadContext context = {.status = SUCCESS,.state = state};

		int parse_error = ini_parse_file(file,read_config_entry,&context);

		status = context.status;

		*has_entries = context.has_entries;

		if(SUCCESS == status && ferror(file))
		{
			argp_failure(state,0,errno,"ERROR: Cannot read config file '%s'",path);
			status = FAILURE;
		}

		if(SUCCESS == status && parse_error != 0)
		{
			argp_failure(state,0,0,"ERROR: Line %d in config file: Invalid config syntax",parse_error);
			status = FAILURE;
		}

		/* Closing remains mandatory after a parse or read error */
		int close_error = fclose(file);

		if(SUCCESS == status && close_error != 0)
		{
			argp_failure(state,0,errno,"ERROR: Cannot close config file '%s'",path);
			status = FAILURE;
		}
	}

	provide(status);
}
