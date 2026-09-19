#include "test_librational_all.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>

struct color_environment_backup {
	char *no_color;
	char *term;
	bool no_color_was_set;
	bool term_was_set;
	bool snapshot_is_valid;
};

static LOGMODES captured_markup_log_level = REGULAR | UNDECOR;
static const char *captured_markup_format = "@{bold}styled@{reset}\n";

/**
 * @brief Write one styled log payload for output capture
 */
static void capture_styled_log(void)
{
	rational_logger(captured_markup_log_level,
		__FILE__,
		__LINE__,
		__func__,
		captured_markup_format);
}

/**
 * @brief Write a styled log payload after a prefix that sanitization expands
 */
static void capture_styled_log_with_unsafe_prefix(void)
{
	rational_logger(VERBOSE,
		"\033unsafe.c",
		7U,
		"unsafe_prefix",
		"@{bold}styled@{reset}\n");
}

/**
 * @brief Write markup cases that exercise conversions and literal markers
 */
static void capture_markup_edge_cases(void)
{
	slog(REGULAR|UNDECOR,"left @{red}%.*s@{reset} %zu\n",3,"value",(size_t)7U);
	slog(REGULAR|UNDECOR,"%s\n","@{red}from data@{reset}");
	slog(REGULAR|UNDECOR,"@{unknown}unknown\n");
	slog(REGULAR|UNDECOR,"@@{red}literal@{reset}\n");
	slog(REGULAR|UNDECOR,"@@@{red}literal@{reset}\n");
	slog(REGULAR|UNDECOR,"@{bold}no automatic reset\n");
	slog(REGULAR|UNDECOR,"@{red}trailing\n@{reset}");
	slog(REGULAR|UNDECOR,"%2$s@{red}%1$s@{reset}\n","first","second");
	slog(REGULAR|UNDECOR,"@{bold}%s","dynamic\n");
	slog(REGULAR|UNDECOR,
		"@{bold}%s@{reset}\n",
		BOLD "allowed" RESET "\033[2Jblocked");
	slog(REGULAR|UNDECOR,"@{red}@{red}repeated@{reset}@{reset}\n");
	slog(REGULAR|UNDECOR,"@{unknown:@{red}}nested@{reset}\n");
	slog(REGULAR|UNDECOR,"@{RED}case sensitive; @{red\n");
	slog(REGULAR|UNDECOR,"@{red}end without reset");
}

/**
 * @brief Write every supported marker with explicit resets where needed
 */
static void capture_all_markup_markers(void)
{
	slog(REGULAR|UNDECOR,
		"@{shorttab}shorttab\n"
		"@{reset}reset\n"
		"@{black}black@{reset}\n"
		"@{gray}gray@{reset}\n"
		"@{red}red@{reset}\n"
		"@{green}green@{reset}\n"
		"@{yellow}yellow@{reset}\n"
		"@{blue}blue@{reset}\n"
		"@{magenta}magenta@{reset}\n"
		"@{cyan}cyan@{reset}\n"
		"@{white}white@{reset}\n"
		"@{bold}bold@{reset}\n"
		"@{boldblack}boldblack@{reset}\n"
		"@{boldred}boldred@{reset}\n"
		"@{boldgreen}boldgreen@{reset}\n"
		"@{boldyellow}boldyellow@{reset}\n"
		"@{boldblue}boldblue@{reset}\n"
		"@{boldmagenta}boldmagenta@{reset}\n"
		"@{boldcyan}boldcyan@{reset}\n"
		"@{boldwhite}boldwhite@{reset}\n");
}

/**
 * @brief Write one markup log while stdout points to a selected stream
 *
 * @param stream Stream whose descriptor temporarily replaces stdout
 * @return SUCCESS when the log was written and stdout was restored
 */
static Return write_markup_log_to_stream(FILE *stream)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int saved_stdout_descriptor = -1;

	if(stream == NULL || fflush(stdout) != 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		saved_stdout_descriptor = dup(STDOUT_FILENO);

		if(saved_stdout_descriptor == -1)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && dup2(fileno(stream),STDOUT_FILENO) == -1)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		slog(REGULAR|UNDECOR,"@{bold}styled@{reset}\n");

		if(fflush(stdout) != 0)
		{
			status = FAILURE;
		}
	}

	if(saved_stdout_descriptor != -1)
	{
		if(dup2(saved_stdout_descriptor,STDOUT_FILENO) == -1)
		{
			status = FAILURE;
		}

		(void)close(saved_stdout_descriptor);
	}

	provide(status);
}

/**
 * @brief Capture a styled payload written to a pipe
 *
 * @param output Storage for the captured bytes and a terminating null byte
 * @param output_capacity Available bytes in output
 * @return SUCCESS when the complete payload was captured, otherwise FAILURE
 */
static Return capture_style_write_from_pipe(
	char   *output,
	size_t output_capacity)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int pipe_descriptors[2] = {-1,-1};
	FILE *pipe_stream = NULL;
	size_t output_length = 0U;

	if(output == NULL || output_capacity == 0U)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && pipe(pipe_descriptors) != 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		pipe_stream = fdopen(pipe_descriptors[1],"w");

		if(pipe_stream == NULL)
		{
			status = FAILURE;
		} else {
			pipe_descriptors[1] = -1;
		}
	}

	if(SUCCESS == status)
	{
		status = write_markup_log_to_stream(pipe_stream);

		if(fflush(pipe_stream) != 0)
		{
			status = FAILURE;
		}
	}

	if(pipe_stream != NULL)
	{
		if(fclose(pipe_stream) != 0)
		{
			status = FAILURE;
		}

		pipe_stream = NULL;
	}

	while(SUCCESS == status && output_length + 1U < output_capacity)
	{
		const ssize_t bytes_read = read(
			pipe_descriptors[0],
			output + output_length,
			output_capacity - output_length - 1U);

		if(bytes_read > 0)
		{
			output_length += (size_t)bytes_read;
		} else if(bytes_read == 0){
			break;
		} else if(errno != EINTR){
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		output[output_length] = '\0';

		if(output_length + 1U == output_capacity)
		{
			char extra_byte = '\0';
			const ssize_t extra_bytes_read = read(pipe_descriptors[0],&extra_byte,sizeof(extra_byte));

			if(extra_bytes_read != 0)
			{
				status = FAILURE;
			}
		}
	}

	if(pipe_stream != NULL)
	{
		(void)fclose(pipe_stream);
	} else if(pipe_descriptors[1] != -1){
		(void)close(pipe_descriptors[1]);
	}

	if(pipe_descriptors[0] != -1)
	{
		(void)close(pipe_descriptors[0]);
	}

	provide(status);
}

/**
 * @brief Capture a styled payload sent through a pseudoterminal
 *
 * @param output Storage for the captured bytes and a terminating null byte
 * @param output_capacity Available bytes in output
 * @return SUCCESS when one complete line was captured, otherwise FAILURE
 */
static Return capture_style_write_from_terminal(
	char   *output,
	size_t output_capacity)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int terminal_descriptor = -1;
	int terminal_peer_descriptor = -1;
	FILE *terminal_stream = NULL;
	size_t output_length = 0U;

	if(output == NULL || output_capacity == 0U)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		terminal_descriptor = posix_openpt(O_RDWR | O_NOCTTY);

		if(terminal_descriptor == -1)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && (grantpt(terminal_descriptor) != 0 || unlockpt(terminal_descriptor) != 0))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const char *terminal_peer_path = ptsname(terminal_descriptor);

		if(terminal_peer_path == NULL)
		{
			status = FAILURE;
		} else {
			terminal_peer_descriptor = open(terminal_peer_path,O_RDWR | O_NOCTTY);

			if(terminal_peer_descriptor == -1)
			{
				status = FAILURE;
			}
		}
	}

	if(SUCCESS == status)
	{
		struct termios terminal_attributes;

		if(tcgetattr(terminal_peer_descriptor,&terminal_attributes) != 0)
		{
			status = FAILURE;
		} else {
			cfmakeraw(&terminal_attributes);

			if(tcsetattr(terminal_peer_descriptor,TCSANOW,&terminal_attributes) != 0)
			{
				status = FAILURE;
			}
		}
	}

	if(SUCCESS == status)
	{
		terminal_stream = fdopen(terminal_peer_descriptor,"w");

		if(terminal_stream == NULL)
		{
			status = FAILURE;
		} else {
			terminal_peer_descriptor = -1;
		}
	}

	if(SUCCESS == status)
	{
		status = write_markup_log_to_stream(terminal_stream);

		if(fflush(terminal_stream) != 0)
		{
			status = FAILURE;
		}
	}

	while(SUCCESS == status && output_length + 1U < output_capacity)
	{
		struct pollfd terminal_poll = {
			.fd = terminal_descriptor,
			.events = POLLIN
		};
		int poll_result = -1;

		do
		{
			poll_result = poll(&terminal_poll,1,5000);
		} while(poll_result == -1 && errno == EINTR);

		if(poll_result <= 0 || (terminal_poll.revents & (POLLIN | POLLHUP)) == 0)
		{
			status = FAILURE;
			break;
		}

		const ssize_t bytes_read = read(
			terminal_descriptor,
			output + output_length,
			output_capacity - output_length - 1U);

		if(bytes_read > 0)
		{
			output_length += (size_t)bytes_read;

			if(output[output_length - 1U] == '\n')
			{
				break;
			}
		} else if(bytes_read == 0){
			status = FAILURE;
		} else if(errno != EINTR){
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		output[output_length] = '\0';

		if(output_length == 0U || output[output_length - 1U] != '\n')
		{
			status = FAILURE;
		}
	}

	if(terminal_stream != NULL)
	{
		(void)fclose(terminal_stream);
	} else if(terminal_peer_descriptor != -1){
		(void)close(terminal_peer_descriptor);
	}

	if(terminal_descriptor != -1)
	{
		(void)close(terminal_descriptor);
	}

	provide(status);
}

/**
 * @brief Save environment settings that affect automatic terminal styles
 *
 * @param backup Storage for duplicated environment values
 * @return SUCCESS when the environment values were saved, otherwise FAILURE
 */
static Return color_environment_save(struct color_environment_backup *backup)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *no_color = getenv("NO_COLOR");
	const char *term = getenv("TERM");

	memset(backup,0,sizeof(*backup));

	if(no_color != NULL)
	{
		backup->no_color = strdup(no_color);

		if(backup->no_color == NULL)
		{
			status = FAILURE;
		} else {
			backup->no_color_was_set = true;
		}
	}

	if(SUCCESS == status && term != NULL)
	{
		backup->term = strdup(term);

		if(backup->term == NULL)
		{
			status = FAILURE;
		} else {
			backup->term_was_set = true;
		}
	}

	if(SUCCESS == status)
	{
		backup->snapshot_is_valid = true;
	}

	provide(status);
}

/**
 * @brief Restore environment settings saved before a terminal style test
 *
 * @param backup Saved environment values to restore and release
 * @return SUCCESS when both settings were restored, otherwise FAILURE
 */
static Return color_environment_restore(struct color_environment_backup *backup)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(true == backup->snapshot_is_valid)
	{
		if(true == backup->no_color_was_set)
		{
			if(0 != setenv("NO_COLOR",backup->no_color,1))
			{
				status = FAILURE;
			}
		} else if(0 != unsetenv("NO_COLOR")){
			status = FAILURE;
		}

		if(true == backup->term_was_set)
		{
			if(0 != setenv("TERM",backup->term,1))
			{
				status = FAILURE;
			}
		} else if(0 != unsetenv("TERM")){
			status = FAILURE;
		}
	}

	free(backup->term);
	free(backup->no_color);
	backup->term = NULL;
	backup->no_color = NULL;

	provide(status);
}

/**
 * @brief Check terminal style modes, environment overrides, and stream types
 *
 * @return Return describing success or failure
 */
static Return test_librational_0006_1(void)
{
	INITTEST;

	struct color_environment_backup environment_backup;
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	FILE *regular_file = NULL;
	FILE *pipe_stream = NULL;
	FILE *terminal_stream = NULL;
	int pipe_descriptors[2] = {-1,-1};
	int terminal_descriptor = -1;

	run(color_environment_save(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","xterm",1));

	regular_file = tmpfile();
	ASSERT(regular_file != NULL);
	ASSERT(0 == pipe(pipe_descriptors));

	if(pipe_descriptors[1] != -1)
	{
		pipe_stream = fdopen(pipe_descriptors[1],"w");
	}

	ASSERT(pipe_stream != NULL);

	if(pipe_stream != NULL)
	{
		pipe_descriptors[1] = -1;
	}

	terminal_descriptor = posix_openpt(O_RDWR | O_NOCTTY);
	ASSERT(terminal_descriptor != -1);

	if(terminal_descriptor != -1)
	{
		terminal_stream = fdopen(terminal_descriptor,"w");
	}

	ASSERT(terminal_stream != NULL);

	if(terminal_stream != NULL)
	{
		terminal_descriptor = -1;
	}

	rational_color_mode = COLOR_MODE_AUTO;
	ASSERT(false == rational_color_is_enabled(NULL));
	ASSERT(false == rational_color_is_enabled(regular_file));
	ASSERT(false == rational_color_is_enabled(pipe_stream));
	ASSERT(true == rational_color_is_enabled(terminal_stream));
	errno = EDOM;
	ASSERT(false == rational_color_is_enabled(pipe_stream));
	ASSERT(EDOM == errno);

	ASSERT(0 == setenv("NO_COLOR","",1));
	ASSERT(false == rational_color_is_enabled(terminal_stream));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","dumb",1));
	ASSERT(false == rational_color_is_enabled(terminal_stream));

	ASSERT(0 == setenv("NO_COLOR","",1));
	rational_color_mode = COLOR_MODE_ALWAYS;
	ASSERT(false == rational_color_is_enabled(NULL));
	ASSERT(true == rational_color_is_enabled(regular_file));
	ASSERT(true == rational_color_is_enabled(terminal_stream));

	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","xterm",1));
	rational_color_mode = COLOR_MODE_NEVER;
	ASSERT(false == rational_color_is_enabled(terminal_stream));

	rational_color_mode = (RATIONAL_COLOR_MODE)0xffU;
	ASSERT(false == rational_color_is_enabled(terminal_stream));

	if(terminal_stream != NULL)
	{
		(void)fclose(terminal_stream);
	} else if(terminal_descriptor != -1){
		(void)close(terminal_descriptor);
	}

	if(pipe_stream != NULL)
	{
		(void)fclose(pipe_stream);
	} else if(pipe_descriptors[1] != -1){
		(void)close(pipe_descriptors[1]);
	}

	if(pipe_descriptors[0] != -1)
	{
		(void)close(pipe_descriptors[0]);
	}

	if(regular_file != NULL)
	{
		(void)fclose(regular_file);
	}

	rational_color_mode = initial_color_mode;
	call(color_environment_restore(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check that automatic style detection follows descriptor redirection
 *
 * @return Return describing success or failure
 */
static Return test_librational_0006_2(void)
{
	INITTEST;

	struct color_environment_backup environment_backup;
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	int saved_stdout_descriptor = -1;
	int saved_stderr_descriptor = -1;
	int terminal_descriptor = -1;
	FILE *regular_file = NULL;

	run(color_environment_save(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","xterm",1));

	saved_stdout_descriptor = dup(STDOUT_FILENO);
	saved_stderr_descriptor = dup(STDERR_FILENO);
	terminal_descriptor = posix_openpt(O_RDWR | O_NOCTTY);
	regular_file = tmpfile();
	ASSERT(saved_stdout_descriptor != -1);
	ASSERT(saved_stderr_descriptor != -1);
	ASSERT(terminal_descriptor != -1);
	ASSERT(regular_file != NULL);
	ASSERT(0 == fflush(stdout));
	ASSERT(0 == fflush(stderr));

	if(SUCCESS == status && terminal_descriptor != -1)
	{
		ASSERT(dup2(terminal_descriptor,STDOUT_FILENO) != -1);
	}

	if(SUCCESS == status && regular_file != NULL)
	{
		ASSERT(dup2(fileno(regular_file),STDERR_FILENO) != -1);
	}

	rational_color_mode = COLOR_MODE_AUTO;
	ASSERT(true == rational_color_is_enabled(stdout));
	ASSERT(false == rational_color_is_enabled(stderr));

	if(SUCCESS == status && regular_file != NULL)
	{
		ASSERT(dup2(fileno(regular_file),STDOUT_FILENO) != -1);
	}

	if(SUCCESS == status && terminal_descriptor != -1)
	{
		ASSERT(dup2(terminal_descriptor,STDERR_FILENO) != -1);
	}

	ASSERT(false == rational_color_is_enabled(stdout));
	ASSERT(true == rational_color_is_enabled(stderr));

	if(0 != fflush(stdout))
	{
		testitall_failure_location_record(__FILE__,__func__,__LINE__);
		status = FAILURE;
	}

	if(0 != fflush(stderr))
	{
		testitall_failure_location_record(__FILE__,__func__,__LINE__);
		status = FAILURE;
	}

	if(saved_stdout_descriptor != -1)
	{
		if(dup2(saved_stdout_descriptor,STDOUT_FILENO) == -1)
		{
			testitall_failure_location_record(__FILE__,__func__,__LINE__);
			status = FAILURE;
		}

		(void)close(saved_stdout_descriptor);
	}

	if(saved_stderr_descriptor != -1)
	{
		if(dup2(saved_stderr_descriptor,STDERR_FILENO) == -1)
		{
			testitall_failure_location_record(__FILE__,__func__,__LINE__);
			status = FAILURE;
		}

		(void)close(saved_stderr_descriptor);
	}

	if(terminal_descriptor != -1)
	{
		(void)close(terminal_descriptor);
	}

	if(regular_file != NULL)
	{
		(void)fclose(regular_file);
	}

	rational_color_mode = initial_color_mode;
	call(color_environment_restore(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check the bytes produced by terminal style modes and overrides
 *
 * @return Return describing success or failure
 */
static Return test_librational_0006_3(void)
{
	INITTEST;

	struct color_environment_backup environment_backup = {0};
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	static const char expected_styled_output[] = BOLD "styled" RESET "\n";
	static const char expected_combined_style_output[] = BOLD RED "styled" RESET "\n";
	static const char expected_prefixed_style_output[] = "TESTING:" BOLD "styled" RESET "\n";
	static const char expected_sanitized_prefix_suffix[] = "\\x1Bunsafe.c:007:unsafe_prefix:" BOLD "styled" RESET "\n";
	static const char expected_plain_output[] = "styled\n";
	char pipe_output[64] = {0};
	char terminal_output[64] = {0};

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	run(color_environment_save(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","xterm",1));
	rational_logger_mode = REGULAR;
	captured_markup_log_level = REGULAR | UNDECOR;
	captured_markup_format = "@{bold}styled@{reset}\n";

	rational_color_mode = COLOR_MODE_ALWAYS;
	ASSERT(SUCCESS == function_capture(
		capture_styled_log,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_styled_output));
	ASSERT(captured_stderr->length == 0U);

	captured_markup_format = "@{bold}@{red}styled@{reset}\n";
	ASSERT(SUCCESS == function_capture(
		capture_styled_log,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_combined_style_output));
	ASSERT(captured_stderr->length == 0U);

	rational_logger_mode = TESTING;
	captured_markup_log_level = TESTING;
	captured_markup_format = "@{bold}styled@{reset}\n";
	ASSERT(SUCCESS == function_capture(
		capture_styled_log,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_prefixed_style_output));
	ASSERT(captured_stderr->length == 0U);

	rational_logger_mode = VERBOSE;
	ASSERT(SUCCESS == function_capture(
		capture_styled_log_with_unsafe_prefix,
		captured_stdout,
		captured_stderr));
	ASSERT(NULL != strstr(m_text(captured_stdout),expected_sanitized_prefix_suffix));
	ASSERT(captured_stderr->length == 0U);

	rational_logger_mode = REGULAR;
	captured_markup_log_level = REGULAR | UNDECOR;
	captured_markup_format = "@{bold}styled@{reset}\n";

	rational_color_mode = COLOR_MODE_NEVER;
	ASSERT(SUCCESS == function_capture(
		capture_styled_log,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_plain_output));
	ASSERT(NULL == strchr(m_text(captured_stdout),'\033'));
	ASSERT(NULL == strstr(m_text(captured_stdout),RESET));
	ASSERT(captured_stderr->length == 0U);

	rational_color_mode = COLOR_MODE_ALWAYS;
	ASSERT(SUCCESS == capture_style_write_from_pipe(pipe_output,sizeof(pipe_output)));
	ASSERT(0 == strcmp(pipe_output,expected_styled_output));

	rational_color_mode = COLOR_MODE_NEVER;
	ASSERT(SUCCESS == capture_style_write_from_terminal(terminal_output,sizeof(terminal_output)));
	ASSERT(0 == strcmp(terminal_output,expected_plain_output));
	ASSERT(NULL == strchr(terminal_output,'\033'));
	ASSERT(NULL == strstr(terminal_output,RESET));

	rational_color_mode = COLOR_MODE_AUTO;
	ASSERT(SUCCESS == capture_style_write_from_pipe(pipe_output,sizeof(pipe_output)));
	ASSERT(0 == strcmp(pipe_output,expected_plain_output));
	ASSERT(NULL == strchr(pipe_output,'\033'));
	ASSERT(NULL == strstr(pipe_output,RESET));

	ASSERT(SUCCESS == capture_style_write_from_terminal(terminal_output,sizeof(terminal_output)));
	ASSERT(0 == strcmp(terminal_output,expected_styled_output));

	ASSERT(0 == setenv("NO_COLOR","",1));
	ASSERT(SUCCESS == capture_style_write_from_terminal(terminal_output,sizeof(terminal_output)));
	ASSERT(0 == strcmp(terminal_output,expected_plain_output));
	ASSERT(NULL == strchr(terminal_output,'\033'));
	ASSERT(NULL == strstr(terminal_output,RESET));

	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","dumb",1));
	ASSERT(SUCCESS == capture_style_write_from_terminal(terminal_output,sizeof(terminal_output)));
	ASSERT(0 == strcmp(terminal_output,expected_plain_output));
	ASSERT(NULL == strchr(terminal_output,'\033'));
	ASSERT(NULL == strstr(terminal_output,RESET));

	captured_markup_log_level = REGULAR | UNDECOR;
	captured_markup_format = "@{bold}styled@{reset}\n";
	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	call(color_environment_restore(&environment_backup));
	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check literal marker replacement after formatting and sanitization
 *
 * @return Return describing success or failure
 */
static Return test_librational_0006_4(void)
{
	INITTEST;

	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	static const char expected_output[] =
	        "left " RED "val" RESET " 7\n"
	        RED "from data" RESET "\n"
	        "@{unknown}unknown\n"
	        "@" RED "literal" RESET "\n"
	        "@@" RED "literal" RESET "\n"
	        BOLD "no automatic reset\n"
	        RED "trailing\n" RESET
	        "second" RED "first" RESET "\n"
	        BOLD "dynamic\n"
	        BOLD "\\x1B[1mallowed\\x1B[0m\\x1B[2Jblocked" RESET "\n"
	        RED RED "repeated" RESET RESET "\n"
	        "@{unknown:" RED "}nested" RESET "\n"
	        "@{RED}case sensitive; @{red\n"
	        RED "end without reset";
	static const char expected_plain_output[] =
	        "left val 7\n"
	        "from data\n"
	        "@{unknown}unknown\n"
	        "@literal\n"
	        "@@literal\n"
	        "no automatic reset\n"
	        "trailing\n"
	        "secondfirst\n"
	        "dynamic\n"
	        "\\x1B[1mallowed\\x1B[0m\\x1B[2Jblocked\n"
	        "repeated\n"
	        "@{unknown:}nested\n"
	        "@{RED}case sensitive; @{red\n"
	        "end without reset";

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	rational_logger_mode = REGULAR;
	rational_color_mode = COLOR_MODE_ALWAYS;
	ASSERT(SUCCESS == function_capture(
		capture_markup_edge_cases,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_output));
	ASSERT(captured_stderr->length == 0U);

	rational_color_mode = COLOR_MODE_NEVER;
	ASSERT(SUCCESS == function_capture(
		capture_markup_edge_cases,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_plain_output));
	ASSERT(captured_stderr->length == 0U);

	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check all marker replacements in colored and plain output
 *
 * @return Return describing success or failure
 */
static Return test_librational_0006_5(void)
{
	INITTEST;

	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	static const char expected_output[] =
	        SHORTTAB "shorttab\n"
	        RESET "reset\n"
	        BLACK "black" RESET "\n"
	        GRAY "gray" RESET "\n"
	        RED "red" RESET "\n"
	        GREEN "green" RESET "\n"
	        YELLOW "yellow" RESET "\n"
	        BLUE "blue" RESET "\n"
	        MAGENTA "magenta" RESET "\n"
	        CYAN "cyan" RESET "\n"
	        WHITE "white" RESET "\n"
	        BOLD "bold" RESET "\n"
	        BOLDBLACK "boldblack" RESET "\n"
	        BOLDRED "boldred" RESET "\n"
	        BOLDGREEN "boldgreen" RESET "\n"
	        BOLDYELLOW "boldyellow" RESET "\n"
	        BOLDBLUE "boldblue" RESET "\n"
	        BOLDMAGENTA "boldmagenta" RESET "\n"
	        BOLDCYAN "boldcyan" RESET "\n"
	        BOLDWHITE "boldwhite" RESET "\n";
	static const char expected_plain_output[] =
	        "shorttab\nreset\nblack\ngray\nred\ngreen\nyellow\nblue\nmagenta\ncyan\nwhite\n"
	        "bold\nboldblack\nboldred\nboldgreen\nboldyellow\nboldblue\nboldmagenta\nboldcyan\nboldwhite\n";

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	rational_logger_mode = REGULAR;
	rational_color_mode = COLOR_MODE_ALWAYS;
	ASSERT(SUCCESS == function_capture(
		capture_all_markup_markers,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_output));
	ASSERT(captured_stderr->length == 0U);

	rational_color_mode = COLOR_MODE_NEVER;
	ASSERT(SUCCESS == function_capture(
		capture_all_markup_markers,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_plain_output));
	ASSERT(captured_stderr->length == 0U);

	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check literal replacement sizes, allocation, and nonrecursive matches
 *
 * @return Return describing success or failure
 */
static Return test_librational_0006_6(void)
{
	INITTEST;

	char *text = strdup("one one one");
	char *unchanged_address = text;
	size_t text_length = sizeof("one one one") - 1U;

	ASSERT(text != NULL);
	ASSERT(true == rational_logger_replace_all(&text,&text_length,"one","x"));
	ASSERT(text == unchanged_address);
	ASSERT(0 == strcmp(text,"x x x"));
	ASSERT(5U == text_length);

	ASSERT(true == rational_logger_replace_all(&text,&text_length,"x","y"));
	ASSERT(text == unchanged_address);
	ASSERT(0 == strcmp(text,"y y y"));
	ASSERT(5U == text_length);

#ifndef EVIL_EMPIRE_OS
	if(SUCCESS == status)
	{
		testmocking_realloc_fail_next(1);
		const bool replacement_succeeded = rational_logger_replace_all(&text,&text_length,"y","red");
		testmocking_realloc_disable();
		ASSERT(false == replacement_succeeded);
		ASSERT(text == unchanged_address);
		ASSERT(0 == strcmp(text,"y y y"));
		ASSERT(5U == text_length);
	}
#endif

	ASSERT(true == rational_logger_replace_all(&text,&text_length,"y","red"));
	ASSERT(0 == strcmp(text,"red red red"));
	ASSERT(11U == text_length);

	ASSERT(true == rational_logger_replace_all(&text,&text_length,"red","redred"));
	ASSERT(0 == strcmp(text,"redred redred redred"));
	ASSERT(20U == text_length);

	unchanged_address = text;
	ASSERT(true == rational_logger_replace_all(&text,&text_length,"missing","longer replacement"));
	ASSERT(text == unchanged_address);
	ASSERT(0 == strcmp(text,"redred redred redred"));
	ASSERT(20U == text_length);
	ASSERT(false == rational_logger_replace_all(&text,&text_length,"","replacement"));
	ASSERT(0 == strcmp(text,"redred redred redred"));
	ASSERT(20U == text_length);

	ASSERT(true == rational_logger_replace_all(&text,&text_length,"redred",""));
	ASSERT(text == unchanged_address);
	ASSERT(0 == strcmp(text,"  "));
	ASSERT(2U == text_length);
	ASSERT(true == rational_logger_replace_all(&text,&text_length," ",""));
	ASSERT(0 == strcmp(text,""));
	ASSERT(0U == text_length);
	ASSERT(true == rational_logger_replace_all(&text,&text_length,"missing","text"));
	ASSERT(0 == strcmp(text,""));
	ASSERT(0U == text_length);
	free(text);

	text = strdup("aaaaa");
	text_length = sizeof("aaaaa") - 1U;
	ASSERT(text != NULL);
	ASSERT(true == rational_logger_replace_all(&text,&text_length,"aa","bbb"));
	ASSERT(0 == strcmp(text,"bbbbbba"));
	ASSERT(7U == text_length);
	free(text);

	RETURN_STATUS;
}

/**
 * @brief Exercise terminal style policy decisions and logger integration
 *
 * @return SUCCESS when all terminal style policy checks pass
 */
Return test_librational_0006(void)
{
	INITTEST;

	TEST(test_librational_0006_1,"terminal colors honor modes, environment settings, files, pipes, and pseudoterminals");
	TEST(test_librational_0006_2,"terminal color detection follows independent stdout and stderr redirection");
	TEST(test_librational_0006_3,"logger markup produces deterministic output for modes, overrides, pipes, and pseudoterminals");
	TEST(test_librational_0006_4,"formatted markup uses literal replacement without escaping or automatic resets");
	TEST(test_librational_0006_5,"all supported markers produce exact colored and plain output");
	TEST(test_librational_0006_6,"literal replacement grows, shrinks, and preserves nonrecursive matching");

	RETURN_STATUS;
}
