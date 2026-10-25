#include "precizer.h"
#include <sysexits.h>

/**
 * @brief Select the settings source and establish CLI logger mode and color
 *
 * The shared option table lets argp distinguish options from their values.
 * Lifecycle events do not count as arguments. Output format, color, silent,
 * and verbose options use the shared handler to select logger mode and color
 * from the requested settings before startup messages. Invalid output and color
 * values are left to the main pass for diagnostics. Other working settings remain
 * unchanged. Information requests are recorded for source selection; the main
 * parser handles their output. A nonzero quoted value rejects the invocation
 * silently so no file is read and the main parser reports it
 *
 * @param key Option key or argp event
 * @param arg Option value or positional argument
 * @param state Preliminary parser state with ArgumentParseContext in input
 * @return EX_OK on success, EX_USAGE for argp-marked --, or ARGP_ERR_UNKNOWN for bulk operands
 */
error_t parse_source_option(
	int               key,
	char              *arg,
	struct argp_state *state)
{
	ArgumentParseContext *context = state->input;

	switch(key)
	{
		case 'g':
			/* Repeated selections replace the path; no file has been opened */
			config->config_file_path = arg;
			break;
		case 's':
		case 'v':
			context->has_other_arguments = true;
			return(parse_opt(key,arg,state));
		case 'O':
			context->has_other_arguments = true;

			/* Only valid values reach the shared handler during this silent pass */
			if(strcasecmp(arg,"full") == 0 || strcasecmp(arg,"compact") == 0 || strcasecmp(arg,"machine") == 0)
			{
				return(parse_opt(key,arg,state));
			}
			break;
		case 'S':
			context->has_other_arguments = true;

			/* Apply valid explicit colors before any startup output */
			if(strcasecmp(arg,"auto") == 0 || strcasecmp(arg,"always") == 0 || strcasecmp(arg,"never") == 0)
			{
				return(parse_opt(key,arg,state));
			}
			break;
		case 'h':
		case '?':
		case 'z':
		case 'V':
			/* Information requests bypass file selection; output stays in the main pass */
			context->information_mode_requested = true;
			context->has_other_arguments = true;
			break;
		case ARGP_KEY_END:
			/* Rely on argp's marker without distinguishing option values from delimiters */
			if(state->quoted != 0)
			{
				return(EX_USAGE);
			}
			break;
		case ARGP_KEY_SUCCESS:
			/* Argp has initialized the diagnostic name and stream by this event.
			   Retain only these fields: the name refers to argv storage, and the
			   stream remains open after this pass. The parser state itself expires
			   when argp_parse() returns and must not be retained */
			context->diagnostic_state.name = state->name;
			context->diagnostic_state.err_stream = state->err_stream;
			break;
		case ARGP_KEY_INIT:
		case ARGP_KEY_NO_ARGS:
		case ARGP_KEY_ERROR:
		case ARGP_KEY_FINI:
			break;
		case ARGP_KEY_ARGS:
			return(ARGP_ERR_UNKNOWN);
		default:
			/* Every other option and each positional operand selects CLI settings */
			context->has_other_arguments = true;
			break;
	}

	return(EX_OK);
}

/**
 * @brief Select a configuration file or CLI-only parsing without opening files
 *
 * Config supplies the last explicit config path; the context records whether
 * other CLI arguments were found. Information requests discard the path and use
 * CLI only. Otherwise, an explicit path conflicts with any other arguments.
 * Without an explicit path, only an argument-free invocation selects the
 * optional default file. A preliminary parse error discards source-selection
 * results silently so the main CLI parser can report the error. Any logger
 * mode already selected remains active.
 * Paths are borrowed from argv or static storage; this function does not copy
 * or validate them. An empty explicit path still selects a required file
 *
 * @param argc Original CLI argument count, including the program name
 * @param preliminary_error Return code from preliminary argp parsing
 * @param[in,out] context Preliminary flags; receives whether the selected file is required
 * @return SUCCESS for a selected source or deferred CLI error, FAILURE for a source conflict
 */
Return select_settings_source(
	const int            argc,
	error_t              preliminary_error,
	ArgumentParseContext *context)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Start with no requirement for a file to exist. Only an explicit config
	   path accepted without conflicting arguments makes the file required */
	context->config_required = false;

	if(preliminary_error != EX_OK)
	{
		/* Preliminary parsing may have collected only part of the arguments.
		   Discard source-selection results so no file is selected. Leave SUCCESS and print
		   nothing here; the main CLI parser must diagnose the original arguments */
		config->config_file_path = NULL;
		context->has_other_arguments = false;
		context->information_mode_requested = false;

	} else if(context->information_mode_requested){
		/* Ignore even an explicit file and leave information output and CLI
		   value validation to the main parser */
		config->config_file_path = NULL;

	} else if(config->config_file_path != NULL){

		/* A non-NULL path records an explicit --config or -g selection.
		   Even an empty string counts as explicit; the reader handles opening */
		if(context->has_other_arguments)
		{
			/* An explicit file cannot be combined with other CLI arguments.
			   Report the conflict and clear the path before any file is opened.
			   FAILURE tells the caller to stop instead of applying either source */
			argp_failure(&context->diagnostic_state,0,0,"ERROR: --config cannot be combined with other arguments. Use either a config file or CLI arguments");
			config->config_file_path = NULL;
			status = FAILURE;

		} else {
			/* Keep the explicit path as the sole settings source. Its absence
			   must be reported as an error when the reader attempts to open it */
			context->config_required = true;
		}

	} else if(argc == 1){
		/* argc includes the program name, so 1 means no CLI arguments.
		   Select the default file in the working directory and keep it optional */
		config->config_file_path = APP_NAME ".conf";
	}

	/* With other CLI arguments and no explicit config path, the path stays
	   NULL and only CLI settings apply. Return the source-selection status */
	deliver(status);
}
