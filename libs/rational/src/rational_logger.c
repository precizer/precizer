#include "rational.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Global flag to manage output of all logging messages
// in an application and its default value
_Atomic LOGMODES rational_logger_mode = REGULAR;
_Atomic Return global_return_status = SUCCESS;

/**
 * @brief Converts LOGMODES bit flags to their string representation
 *
 * @details This function takes a combination of LOGMODES flags and converts them
 *          into a human-readable string representation where individual flags
 *          are separated by " | ". For example, (VERBOSE | SILENT) will be
 *          converted to "VERBOSE | SILENT"
 *
 * @param mode Combination of LOGMODES flags
 * @return char* Pointer to static string containing flag names
 *
 * @note The function uses a static buffer which means:
 *       1. No memory allocation/deallocation is needed
 *       2. The buffer contents will be overwritten on next function call
 *       3. The function is not thread-safe
 *       4. The returned pointer should not be freed
 *
 * @warning Maximum resulting string length is limited to 256 characters
 */
char *rational_reconvert(LOGMODES mode)
{
	/* Static buffer to store the resulting string */
	static char buffer[MAX_CHARACTERS];
	buffer[0] = '\0';  /* Initialize buffer as empty string */

	/* Flag to track if we're adding the first item (for | separator) */
	int first = 1;

	/* Define mapping between flag values and their string representations
	 * The array is terminated with {0, NULL} for easy iteration
	 */
	static const struct {
		LOGMODES flag;     /* Flag value from LOGMODES constants */
		const char *name;  /* String representation of the flag */
	} mapping[] = {
		{REGULAR,"REGULAR"},
		{VERBOSE,"VERBOSE"},
		{TESTING,"TESTING"},
		{ERROR,"ERROR"},
		{SILENT,"SILENT"},
		{UNDECOR,"UNDECOR"},
		{REMEMBER,"REMEMBER"},
		{VISIBLE_IN_SILENT,"VISIBLE_IN_SILENT"},
		{0,NULL}   /* Terminator element */
	};

	/* Iterate through all possible flags */
	for(int i = 0; mapping[i].name != NULL; i++)
	{
		/* Check if current flag is set in mode using bitwise AND */
		if(mode & mapping[i].flag)
		{
			/* Add separator before all elements except the first one */
			if(!first)
			{
				strcat(buffer," | ");
			}

			/* Add flag name to the result string */
			strcat(buffer,mapping[i].name);

			/* Clear first flag as we've added an element */
			first = 0;
		}
	}

	return buffer;
}

/**
 *
 * @brief Format current date and time in ISO format
 * @param time_string Pointer to the destination buffer that receives the timestamp.
 * @param buffer_size Size of the destination buffer in bytes.
 * @return Return SUCCESS on success, FAILURE on error (buffer contents will be empty on failure).
 *
 */
static Return logger_show_time(
	char   *time_string,
	size_t buffer_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	struct timeval current_time;
	struct tm local_time;

	if(gettimeofday(&current_time,NULL) != 0)
	{
		time_string[0] = '\0';
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(localtime_r(&current_time.tv_sec,&local_time) == NULL)
		{
			time_string[0] = '\0';
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		const int milliseconds = (int)(current_time.tv_usec / 1000);

		if(snprintf(time_string,
			buffer_size,
			"%04d-%02d-%02d %02d:%02d:%02d:%03d",
			local_time.tm_year + 1900,
			local_time.tm_mon + 1,
			local_time.tm_mday,
			local_time.tm_hour,
			local_time.tm_min,
			local_time.tm_sec,
			milliseconds) < 0)
		{
			time_string[0] = '\0';
			status = FAILURE;
		}
	}

	return(status);
}

__attribute__((format(printf,3,0)))
static void logger_line_append_va(
	char       **line,
	int        *line_len,
	const char *fmt,
	va_list    args)
{
	va_list args_copy;
	va_copy(args_copy,args);
	const int needed = vsnprintf(NULL,0,fmt,args_copy);
	va_end(args_copy);

	if(needed < 0)
	{
		return;
	}

	const size_t new_len = (size_t)(*line_len) + (size_t)needed;
	char *tmp = realloc(*line,new_len + 1);

	if(tmp == NULL)
	{
		return;
	}

	*line = tmp;

	va_list args_copy2;
	va_copy(args_copy2,args);
	vsnprintf(*line + *line_len,(size_t)needed + 1,fmt,args_copy2);
	va_end(args_copy2);

	*line_len = (int)new_len;
}

__attribute__((format(printf,3,4)))
static void logger_line_append(
	char       **line,
	int        *line_len,
	const char *fmt,
	...)
{
	va_list args;
	va_start(args,fmt);
	logger_line_append_va(line,line_len,fmt,args);
	va_end(args);
}

/**
 * @brief Append one byte as a visible hexadecimal escape
 *
 * @param[in,out] line Destination buffer with enough free space
 * @param[in,out] line_len Current destination length in bytes
 * @param[in] byte Byte to append as \xNN
 */
static void logger_line_append_hex_escape(
	char          *line,
	size_t        *line_len,
	unsigned char byte)
{
	/*
	 * Use an explicit digit table instead of sprintf().
	 * This helper is used from the logger cleanup path, so it should not depend
	 * on formatting functions or temporary buffers just to render one byte
	 */
	static const char hex_digits[] = "0123456789ABCDEF";

	/*
	 * Write the escape directly into the caller-owned output buffer.
	 * The sanitizer allocates enough room before calling this helper, and
	 * line_len is advanced after every written character so the next append can
	 * continue from the correct position
	 */
	line[(*line_len)++] = '\\';
	line[(*line_len)++] = 'x';
	line[(*line_len)++] = hex_digits[byte >> 4U];
	line[(*line_len)++] = hex_digits[byte & 0x0FU];
}

/**
 * @brief Decode one UTF-8 sequence from a bounded byte range
 *
 * @details
 * The logger uses this small decoder instead of locale-dependent multibyte
 * conversion because the process locale is not guaranteed to be initialized
 * before a log line is printed. The function accepts only shortest-form UTF-8,
 * rejects surrogate code points, and rejects values outside the Unicode range
 *
 * @param[in] line Source byte range
 * @param[in] line_len Number of bytes available at @p line
 * @param[out] codepoint Decoded Unicode code point
 * @return Number of bytes consumed, or 0 when the input is not valid UTF-8
 */
static size_t logger_line_decode_utf8(
	const char *line,
	size_t     line_len,
	uint32_t   *codepoint)
{
	/*
	 * Refuse invalid input before looking at the first byte.
	 * Returning 0 tells the caller to treat the current byte as unsafe and
	 * print it as a visible escape instead of trusting it as text
	 */
	if(line == NULL || codepoint == NULL || line_len == 0U)
	{
		return(0U);
	}

	const unsigned char first_byte = (unsigned char)line[0];

	/*
	 * ASCII is a single-byte subset of UTF-8.
	 * Decoding it here keeps the rest of the function focused on multibyte
	 * sequences and lets the caller apply its own ASCII control-character policy
	 */
	if(first_byte < 0x80U)
	{
		*codepoint = (uint32_t)first_byte;
		return(1U);
	}

	/*
	 * Classify the leading byte and prepare the partial code point.
	 * The first byte tells us how many continuation bytes must follow and also
	 * provides the high bits of the decoded Unicode value
	 */
	size_t expected_len = 0U;
	uint32_t decoded_codepoint = 0U;
	uint32_t lowest_codepoint = 0U;

	if(first_byte >= 0xC2U && first_byte <= 0xDFU)
	{
		expected_len = 2U;
		decoded_codepoint = (uint32_t)(first_byte & 0x1FU);
		lowest_codepoint = 0x80U;
	} else if(first_byte >= 0xE0U && first_byte <= 0xEFU){
		expected_len = 3U;
		decoded_codepoint = (uint32_t)(first_byte & 0x0FU);
		lowest_codepoint = 0x800U;
	} else if(first_byte >= 0xF0U && first_byte <= 0xF4U){
		expected_len = 4U;
		decoded_codepoint = (uint32_t)(first_byte & 0x07U);
		lowest_codepoint = 0x10000U;
	} else {
		/*
		 * Bytes outside these leading-byte ranges cannot start a valid UTF-8
		 * sequence. This includes continuation bytes seen without a starter,
		 * obsolete overlong starters, and values beyond the Unicode limit
		 */
		return(0U);
	}

	/*
	 * A valid sequence must be complete inside the provided byte range.
	 * The logger works with bounded buffers, so an incomplete trailing sequence
	 * is escaped byte by byte rather than reading past the formatted line
	 */
	if(line_len < expected_len)
	{
		return(0U);
	}

	/*
	 * Every byte after the leading byte must have the UTF-8 continuation shape
	 * 10xxxxxx. While checking that shape, assemble the final code point by
	 * shifting in the six payload bits carried by each continuation byte
	 */
	for(size_t i = 1U; i < expected_len; i++)
	{
		const unsigned char continuation_byte = (unsigned char)line[i];

		if((continuation_byte & 0xC0U) != 0x80U)
		{
			return(0U);
		}

		decoded_codepoint = (decoded_codepoint << 6U) | (uint32_t)(continuation_byte & 0x3FU);
	}

	/*
	 * Reject overlong encodings.
	 * UTF-8 has exactly one shortest byte representation for each code point,
	 * and accepting longer aliases would let unsafe bytes hide behind another
	 * spelling of the same character
	 */
	if(decoded_codepoint < lowest_codepoint)
	{
		return(0U);
	}

	/*
	 * UTF-16 surrogate values are not Unicode scalar values.
	 * They are invalid in UTF-8 text, so the logger escapes their original bytes
	 * instead of copying them into terminal output
	 */
	if(decoded_codepoint >= 0xD800U && decoded_codepoint <= 0xDFFFU)
	{
		return(0U);
	}

	/*
	 * Unicode ends at U+10FFFF.
	 * Anything above that value is invalid input and must be shown as escaped
	 * bytes, not as trusted text
	 */
	if(decoded_codepoint > 0x10FFFFU)
	{
		return(0U);
	}

	/*
	 * At this point the byte sequence is well-formed UTF-8.
	 * Return both the decoded code point and the number of bytes consumed so the
	 * sanitizer can decide whether the character is safe to copy
	 */
	*codepoint = decoded_codepoint;
	return(expected_len);
}

/**
 * @brief Escape bytes that can disrupt terminal output
 *
 * @details
 * The logger receives an already formatted line, so this layer cannot tell
 * whether bytes came from a file name, a database path, an error message, or
 * fixed application text. The filter therefore treats the complete log line as
 * terminal output and escapes only byte patterns that are unsafe to write as
 * text. Plain printable ASCII, newline, carriage return, tab, raw ESC, and
 * valid non-C1 UTF-8 multibyte sequences are preserved. Invalid multibyte input
 * is escaped byte by byte so malformed file names remain visible without being
 * interpreted by the terminal. Unicode C1 control characters such as U+0090
 * are also escaped, even though their UTF-8 byte sequence is formally valid,
 * because terminals may interpret them as control strings and hide later output.
 *
 * This first pass deliberately leaves raw ESC bytes unchanged. The project uses
 * ESC-based decorations such as bold and colors, and distinguishing those
 * trusted decorations from path bytes requires a separate whitelist policy. That
 * whitelist can be added later without changing slog() call sites
 *
 * @param[in,out] line Pointer to the allocated line buffer
 * @param[in,out] line_len Current line length in bytes, updated on success
 */
static void logger_line_sanitize_for_terminal(
	char **line,
	int  *line_len)
{
	/*
	 * Nothing useful can be sanitized without an existing allocated buffer and a
	 * positive byte count. Returning quietly preserves the logger's current
	 * best-effort behavior for allocation and formatting failure paths
	 */
	if(line == NULL || *line == NULL || line_len == NULL || *line_len <= 0)
	{
		return;
	}

	const size_t input_len = (size_t)*line_len;

	/*
	 * Escaping one input byte as \xNN needs four output bytes.
	 * This guard keeps the worst-case allocation and the final int length update
	 * inside representable bounds
	 */
	if(input_len > (size_t)INT_MAX / 4U)
	{
		return;
	}

	/*
	 * Allocate for the worst case where every input byte becomes \xNN.
	 * The extra byte is for a terminator because the logger stores text in a C
	 * string buffer even though fwrite() uses the explicit byte length
	 */
	char *sanitized_line = malloc((input_len * 4U) + 1U);

	if(sanitized_line == NULL)
	{
		return;
	}

	/*
	 * input_position walks through the original formatted line.
	 * output_len tracks the next free position in the sanitized replacement
	 * buffer, which may grow faster than the input when bytes are escaped
	 */
	size_t input_position = 0U;
	size_t output_len = 0U;

	/*
	 * Consume one ASCII byte or one complete UTF-8 sequence per loop.
	 * The loop never trusts a multibyte sequence until it has been decoded and
	 * checked, so damaged input cannot leak raw terminal controls into output
	 */
	while(input_position < input_len)
	{
		const unsigned char byte = (unsigned char)(*line)[input_position];

		if(byte < 0x80U)
		{
			/*
			 * ASCII printable characters are safe to copy.
			 * Newline, carriage return, and tab are preserved because log lines
			 * legitimately use them for layout. ESC is also preserved for now so
			 * existing color and bold decorations keep working until an explicit
			 * decoration whitelist is added
			 */
			if(byte == '\n' || byte == '\r' || byte == '\t' || byte == '\033' || (byte >= 0x20U && byte != 0x7FU))
			{
				sanitized_line[output_len++] = (char)byte;
			} else {
				/*
				 * Other ASCII control bytes and DEL are not safe terminal text.
				 * Showing them as \xNN makes the byte visible to the user while
				 * preventing the terminal from treating it as an action
				 */
				logger_line_append_hex_escape(sanitized_line,&output_len,byte);
			}

			input_position++;
			continue;
		}

		/*
		 * Non-ASCII input must first prove that it is valid UTF-8.
		 * The decoder is intentionally local and locale-independent so logger
		 * safety does not depend on whether setlocale() has already run
		 */
		uint32_t codepoint = 0U;
		const size_t decoded_len = logger_line_decode_utf8(*line + input_position,
			input_len - input_position,
			&codepoint);

		/*
		 * Invalid UTF-8 is escaped one byte at a time.
		 * This preserves every original byte in a readable form and then retries
		 * from the next byte, which helps recover cleanly after a malformed prefix
		 */
		if(decoded_len == 0U)
		{
			logger_line_append_hex_escape(sanitized_line,&output_len,byte);
			input_position++;
			continue;
		}

		/*
		 * C1 controls are dangerous even when encoded as valid UTF-8.
		 * U+0090, for example, is a terminal control-string introducer on some
		 * terminals, so the original bytes are escaped instead of being copied
		 */
		if(codepoint >= 0x80U && codepoint <= 0x9FU)
		{
			for(size_t i = 0U; i < decoded_len; i++)
			{
				const unsigned char control_byte = (unsigned char)(*line)[input_position + i];
				logger_line_append_hex_escape(sanitized_line,&output_len,control_byte);
			}
		} else {
			/*
			 * Valid non-C1 UTF-8 is copied unchanged.
			 * This keeps ordinary international file names and messages readable
			 * instead of turning every non-ASCII character into escapes
			 */
			memcpy(sanitized_line + output_len,*line + input_position,decoded_len);
			output_len += decoded_len;
		}

		input_position += decoded_len;
	}

	/*
	 * Replace the original formatted line with the sanitized version.
	 * The caller will pass the same buffer to REMEMBER and fwrite(), so both
	 * delayed warnings and immediate terminal output see identical safe text
	 */
	sanitized_line[output_len] = '\0';
	free(*line);
	*line = sanitized_line;
	*line_len = (int)output_len;
}

__attribute__((format(printf,7,0)))
static void logger_line(
	char              **line,
	int               *line_len,
	const LOGMODES    level,
	const char *const filename,
	size_t            line_number,
	const char *const funcname,
	const char        *fmt,
	va_list           args)
{
	if(rational_logger_mode & SILENT)
	{
		if(level & VISIBLE_IN_SILENT)
		{
			logger_line_append_va(line,line_len,fmt,args);
		}

		return;
	}

	if(!(level & UNDECOR) && (level & TESTING) && (rational_logger_mode & TESTING))
	{
		// Print out the word "TESTING:"
		logger_line_append(line,line_len,"TESTING:");
	}

	if(!(level & UNDECOR) && (level & (VERBOSE|ERROR)) && (rational_logger_mode & VERBOSE))
	{
		char time_string[sizeof "2011-10-18 07:07:09:000"];
		(void)logger_show_time(time_string,sizeof(time_string));

		// Print out current time
		logger_line_append(line,line_len,"%s ",time_string);

		// Print out the source file name
		logger_line_append(line,line_len,"%s:",filename);

		// Print out line number in source file
		logger_line_append(line,line_len,"%03zu:",line_number);

		// Print out name of the function itself
		logger_line_append(line,line_len,"%s:",funcname);
	}

	if(!(level & UNDECOR) && (level & ERROR) && (rational_logger_mode & (REGULAR | ERROR)))
	{
		// Print out error prefix
		logger_line_append(line,line_len,"ERROR: ");

	} else if(!(level & UNDECOR) && (level & ERROR) && (rational_logger_mode & (TESTING | VERBOSE))){
		// Print out the word "ERROR:"
		logger_line_append(line,line_len,"ERROR:");
	}

	if(level & ERROR && rational_logger_mode & ERROR)
	{
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);

	} else if(level & (REGULAR|ERROR) && rational_logger_mode & REGULAR){
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);

	} else if(level & (VERBOSE|ERROR) && rational_logger_mode & VERBOSE){
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);

	} else if(level & (TESTING|ERROR) && rational_logger_mode & TESTING){
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);
	}
}

/**
 *
 * @brief Build and print a formatted log line with file, line, and function metadata
 *
 * @details When REMEMBER is set and the weak rational_remember() symbol is defined,
 *          the formatted line (without a trailing newline) and its length are
 *          passed to that callback.
 *
 */
__attribute__((format(printf,5,6))) // Without this we will get warning
void rational_logger(
	const LOGMODES    level,
	const char *const filename,
	size_t            line,
	const char *const funcname,
	const char        *fmt,
	...)
{

	char *logger_line_text = NULL;
	int line_len = 0;

	va_list args;
	va_start(args,fmt);
	logger_line(&logger_line_text,&line_len,level,filename,line,funcname,fmt,args);
	va_end(args);

	/*
	 * Sanitize the final formatted line before any consumer sees it.
	 * This keeps immediate terminal output and delayed REMEMBER output identical,
	 * and prevents path bytes from being interpreted as terminal controls
	 */
	logger_line_sanitize_for_terminal(&logger_line_text,&line_len);

	if((level & REMEMBER) && rational_remember && logger_line_text != NULL && line_len > 0)
	{
		rational_remember(logger_line_text,line_len);
	}

	if(logger_line_text != NULL)
	{
		fwrite(logger_line_text,sizeof(char),(size_t)line_len,stdout);
	}

	free(logger_line_text);
}

#ifdef TEST
/**
 * @file test_slog.c
 * @brief Complete test suite for log functionality
 */
int main(void)
{
	printf("All available combinations:\n");
	printf("%s\n",rational_convert(REGULAR));
	printf("%s\n",rational_convert(VERBOSE));
	printf("%s\n",rational_convert(TESTING));
	printf("%s\n",rational_convert(SILENT));
	printf("%s\n",rational_convert(REGULAR|VERBOSE));
	printf("%s\n",rational_convert(REGULAR|TESTING));
	printf("%s\n",rational_convert(VERBOSE|TESTING));
	printf("%s\n",rational_convert(REGULAR|VERBOSE|TESTING));
	printf("%s\n",rational_convert(ERROR));
	printf("%s\n",rational_convert(UNDECOR));
	printf("%s\n",rational_convert(EVERY|UNDECOR));
	printf("%s\n",rational_convert(ERROR|UNDECOR));
	printf("%s\n",rational_convert(VISIBLE_IN_SILENT));

	/* Test REGULAR mode combinations */
	rational_logger_mode = REGULAR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("1.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("2. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("3. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("4.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test VERBOSE mode combinations */
	rational_logger_mode = VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("5. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("6.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("7. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("8.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test TESTING mode combinations */
	rational_logger_mode = TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("9.  Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("10. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("11.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("12.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test SILENT mode combinations */
	rational_logger_mode = SILENT;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("13. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("14. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("15. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("16. Won't print:");   slog(ERROR,"but printed!"); printf("\n");

	/* Test REGULAR|VERBOSE combinations */
	rational_logger_mode = REGULAR|VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("17.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("18.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("19. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("20.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test REGULAR|TESTING combinations */
	rational_logger_mode = REGULAR|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("21.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("22. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("23.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("24.  Must print:"); slog(ERROR,"true"); printf("\n");

	/* Test VERBOSE|TESTING combinations */
	rational_logger_mode = VERBOSE|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("25. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("26.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("27.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("28.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test REGULAR|VERBOSE|TESTING combinations */
	rational_logger_mode = REGULAR|VERBOSE|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("29. Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("30. Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("31. Must print:"); slog(TESTING,"true"); printf("\n");
	printf("32. Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test ERROR mode combinations */
	rational_logger_mode = ERROR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("33. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("34. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("35. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("36.  Must print:");   slog(ERROR,"true"); printf("\n");

	/*
	 * Test UNDECOR flag: suppress logger prefixes (TESTING:, time/file/line/func, ERROR:)
	 * The output between the '|' markers should contain only the message payload.
	 */

	rational_logger_mode = EVERY|ERROR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("37. Must print no prefixes:|"); slog(EVERY|UNDECOR,"true"); printf("|\n");
	printf("38. Must print no ERROR prefix:|"); slog(ERROR|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("39. Must print no time/file/line/func:|"); slog(VERBOSE|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("40. Must print no TESTING prefix:|"); slog(TESTING|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = REGULAR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("41. Must not print (VERBOSE not enabled):|"); slog(VERBOSE|UNDECOR,"but printed!"); printf("|\n");

	rational_logger_mode = SILENT;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("42. Must print in SILENT without prefixes:|"); slog(EVERY|VISIBLE_IN_SILENT,"true"); printf("|\n");
	printf("43. Must print no ERROR prefix in SILENT:|"); slog(ERROR|VISIBLE_IN_SILENT,"true"); printf("|\n");

	return 0;
}
#endif
