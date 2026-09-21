#include "rational.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Global flag to manage output of all logging messages
// in an application and its default value
_Atomic LOGMODES rational_logger_mode = REGULAR;

/* Keep only the logger's optional reference weak. Program implementations of
   the REMEMBER callback remain strong symbols */
extern void rational_remember(
	const char *,
	const int) __attribute__((weak));

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

/**
 * @brief Append one printf payload while preserving the caller's errno
 *
 * @param[in,out] line Allocated destination line
 * @param[in,out] line_len Current destination length
 * @param[in] fmt Printf format string
 * @param args Arguments associated with @p fmt
 * @return true when the complete payload was appended, otherwise false
 */
__attribute__((format(printf,3,0)))
static bool logger_line_append_va(
	char       **line,
	int        *line_len,
	const char *fmt,
	va_list    args)
{
	if(line == NULL || line_len == NULL || *line_len < 0 || fmt == NULL)
	{
		return(false);
	}

	const int format_errno = errno;
	va_list args_copy;
	va_copy(args_copy,args);
	errno = format_errno;
	const int needed = vsnprintf(NULL,0,fmt,args_copy);
	va_end(args_copy);
	errno = format_errno;

	if(needed < 0 || needed > INT_MAX - *line_len)
	{
		return(false);
	}

	const size_t new_len = (size_t)(*line_len) + (size_t)needed;
	char *tmp = realloc(*line,new_len + 1);

	if(tmp == NULL)
	{
		return(false);
	}

	*line = tmp;

	va_list args_copy2;
	va_copy(args_copy2,args);
	errno = format_errno;
	const int written = vsnprintf(*line + *line_len,(size_t)needed + 1U,fmt,args_copy2);
	va_end(args_copy2);
	errno = format_errno;

	if(written != needed)
	{
		(*line)[*line_len] = '\0';
		return(false);
	}

	*line_len = (int)new_len;
	return(true);
}

/**
 * @brief Append one variadic printf payload to a log line
 *
 * @param[in,out] line Allocated destination line
 * @param[in,out] line_len Current destination length
 * @param[in] fmt Printf format string
 * @return true when the complete payload was appended, otherwise false
 */
__attribute__((format(printf,3,4)))
static bool logger_line_append(
	char       **line,
	int        *line_len,
	const char *fmt,
	...)
{
	va_list args;
	va_start(args,fmt);
	const bool was_appended = logger_line_append_va(line,line_len,fmt,args);
	va_end(args);

	return(was_appended);
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
 * @brief Measure or write text with unsafe terminal bytes escaped
 *
 * @details
 * Each unsafe byte becomes a four-character \xNN escape. Passing NULL as the
 * output buffer measures the result without writing or allocating memory.
 * To write the result, pass a separate buffer with at least the measured size
 * and the same unchanged input. The function does not append a terminator
 *
 * @param[in] input Source bytes, including any embedded null bytes
 * @param[in] input_len Number of bytes available at @p input
 * @param[out] output Destination buffer, or NULL to measure only
 * @return Result length excluding the terminator, or SIZE_MAX when the result
 *         cannot fit in an int or leave room for a terminator in size_t
 */
static size_t logger_line_sanitize_text(
	const char *input,
	size_t     input_len,
	char       *output)
{
	/*
	 * input_position walks through the original formatted line.
	 * output_len tracks the sanitized length in both measurement and writing
	 * modes, and grows faster than the input when bytes are escaped
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
		const unsigned char byte = (unsigned char)input[input_position];
		size_t consumed_length = 1U;
		bool escape_required = false;

		/* Printable ASCII and the permitted layout characters keep their bytes.
		   All other ASCII controls, including embedded null bytes, need escapes */
		if(byte < 0x80U)
		{
			/*
			 * ASCII printable characters are safe to copy.
			 * Newline, carriage return, and tab are preserved because log lines
			 * legitimately use them for layout. Every other control byte remains
			 * visible instead of controlling a terminal
			 */
			if(byte != '\n' && byte != '\r' && byte != '\t'
			        && (byte < 0x20U || byte == 0x7FU))
			{
				/*
				 * Other ASCII control bytes and DEL are not safe terminal text.
				 * Showing them as \xNN makes the byte visible to the user while
				 * preventing the terminal from treating it as an action
				 */
				escape_required = true;
			}
		} else {
			/*
			 * Non-ASCII input must first prove that it is valid UTF-8.
			 * The decoder is intentionally local and locale-independent so logger
			 * safety does not depend on whether setlocale() has already run
			 */
			uint32_t codepoint = 0U;
			const size_t decoded_len = logger_line_decode_utf8(input + input_position,
				input_len - input_position,&codepoint);

			/*
			 * Invalid UTF-8 is escaped one byte at a time.
			 * This preserves every original byte in a readable form and then retries
			 * from the next byte, which helps recover cleanly after a malformed prefix
			 */
			if(decoded_len == 0U)
			{
				escape_required = true;
			} else {
				/* Consume a complete validated UTF-8 sequence. Its bytes are either
				   retained together or individually escaped for a C1 control */
				consumed_length = decoded_len;

				/*
				 * C1 controls are dangerous even when encoded as valid UTF-8.
				 * U+0090, for example, is a terminal control-string introducer on some
				 * terminals, so the original bytes are escaped instead of being copied
				 */
				if(codepoint >= 0x80U && codepoint <= 0x9FU)
				{
					escape_required = true;
				}
			}
		}

		/*
		 * Escaping one input byte as \xNN needs four output bytes.
		 * A fragment consumes at most four input bytes, so its size can safely be
		 * multiplied by four. Check the total before adding the fragment so both
		 * the final int length and the size_t allocation with a terminator fit
		 */
		size_t fragment_length = consumed_length;

		if(escape_required == true)
		{
			fragment_length *= 4U;
		}

		if(fragment_length > (size_t)INT_MAX - output_len
		        || fragment_length > SIZE_MAX - output_len - 1U)
		{
			return(SIZE_MAX);
		}

		/* Measurement and writing use the same fragment lengths. Only the write
		   pass touches the output buffer, which the caller has already sized */
		if(output != NULL)
		{
			if(escape_required == true)
			{
				size_t write_position = output_len;

				for(size_t i = 0U; i < consumed_length; i++)
				{
					logger_line_append_hex_escape(output,&write_position,
						(unsigned char)input[input_position + i]);
				}
			} else {
				/*
				 * Printable ASCII, permitted layout bytes, and valid non-C1 UTF-8
				 * are copied unchanged. This keeps ordinary international file
				 * names and messages readable instead of escaping safe characters
				 */
				memcpy(output + output_len,input + input_position,consumed_length);
			}
		}

		output_len += fragment_length;
		input_position += consumed_length;
	}

	return(output_len);
}

/**
 * @brief Escape bytes that can disrupt terminal output
 *
 * @details
 * The logger receives an already formatted line, so this layer cannot tell
 * whether bytes came from a file name, a database path, an error message, or
 * fixed application text. The filter therefore treats the complete log line as
 * terminal output and escapes only byte patterns that are unsafe to write as
 * text. Plain printable ASCII, newline, carriage return, tab, and valid non-C1
 * UTF-8 multibyte sequences are preserved. Invalid multibyte input and raw ESC
 * bytes are escaped byte by byte so malformed file names remain visible without
 * being interpreted by the terminal. Markup is interpreted only after this
 * function, so every terminal sequence present at this stage is untrusted.
 * Unicode C1 controls are escaped even when their UTF-8 representation is
 * otherwise valid. A sizing pass retains the input buffer when no bytes need
 * escaping; otherwise, an exactly sized buffer receives the sanitized text.
 * Allocation failures or unrepresentable result lengths suppress the line
 *
 * @param[in,out] line Pointer to the allocated line buffer
 * @param[in,out] line_len Current line length in bytes, updated on success
 */
static void logger_line_sanitize_for_terminal(
	char **line,
	int  *line_len)
{
	/*
	 * Sanitization requires an allocated buffer and a positive byte count.
	 * Missing pointers and empty lines return immediately without accessing
	 * the buffer contents
	 */
	if(line == NULL || *line == NULL || line_len == NULL || *line_len <= 0)
	{
		return;
	}

	const size_t input_len = (size_t)*line_len;

	/* Measure the result, counting four characters for each escaped byte.
	   Every other byte keeps its original size, including valid non-C1 UTF-8 */
	const size_t sanitized_length = logger_line_sanitize_text(*line,input_len,NULL);

	if(sanitized_length == SIZE_MAX)
	{
		free(*line);
		*line = NULL;
		*line_len = 0;
		return;
	}

	/* Fully checked text without unsafe bytes already has the required form.
	   Keep its allocation and avoid copying the unchanged message */
	if(sanitized_length == input_len)
	{
		return;
	}

	/*
	 * Allocate exactly the input length plus the growth from escaped bytes.
	 * The extra byte is for a terminator because the logger stores text in a C
	 * string buffer even though fwrite() uses the explicit byte length
	 */
	char *sanitized_line = malloc(sanitized_length + 1U);

	if(sanitized_line == NULL)
	{
		/* Suppress the line instead of allowing untrusted ESC bytes to bypass
		   sanitization when memory is exhausted */
		free(*line);
		*line = NULL;
		*line_len = 0;
		return;
	}

	/* The input is unchanged, so writing produces exactly the measured length */
	(void)logger_line_sanitize_text(*line,input_len,sanitized_line);

	/*
	 * Replace the original formatted line with the sanitized version.
	 * Trusted decoration sequences are inserted only after this step
	 */
	sanitized_line[sanitized_length] = '\0';
	free(*line);
	*line = sanitized_line;
	*line_len = (int)sanitized_length;
}

/**
 * @brief Build the common log prefix and append one payload
 *
 * @details A single logger-mode snapshot controls the entire line. The verbose
 *          prefix is appended as one formatted fragment. An append failure
 *          stops assembly, leaving the caller responsible for freeing the line
 *
 * @param[in,out] line Allocated output line
 * @param[in,out] line_len Current output length
 * @param level Requested logger modes
 * @param[in] filename Source file name
 * @param line_number Source line number
 * @param[in] funcname Source function name
 * @param[in] fmt Original printf format
 * @param args Arguments associated with @p fmt
 * @param format_errno errno value visible to `%m` conversions
 * @return true when the complete line was built, otherwise false
 */
__attribute__((format(printf,7,0)))
static bool logger_line(
	char              **line,
	int               *line_len,
	const LOGMODES    level,
	const char *const filename,
	size_t            line_number,
	const char *const funcname,
	const char        *fmt,
	va_list           args,
	int               format_errno)
{
	if(line == NULL || line_len == NULL)
	{
		return(false);
	}

	/* Use one atomic snapshot so all prefix and payload checks use the same
	   mode even when another thread updates the shared setting */
	const LOGMODES logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	bool payload_is_visible = false;

	if(logger_mode & SILENT)
	{
		if(level & VISIBLE_IN_SILENT)
		{
			payload_is_visible = true;
		}
	} else {
		if(!(level & UNDECOR) && (level & TESTING) && (logger_mode & TESTING))
		{
			// Print out the word "TESTING:"
			if(logger_line_append(line,line_len,"TESTING:") == false)
			{
				return(false);
			}
		}

		if(!(level & UNDECOR) && (level & (VERBOSE|ERROR)) && (logger_mode & VERBOSE))
		{
			char time_string[sizeof "2011-10-18 07:07:09:000"];
			(void)logger_show_time(time_string,sizeof(time_string));

			// Print out current time
			if(logger_line_append(line,line_len,"%s %s:%03zu:%s:",time_string,
				// Print out the source file name
				filename,
				// Print out line number in source file
				line_number,
				// Print out name of the function itself
				funcname) == false)
			{
				return(false);
			}
		}

		if(!(level & UNDECOR) && (level & ERROR) && (logger_mode & (REGULAR | ERROR)))
		{
			// Print out error prefix
			if(logger_line_append(line,line_len,"ERROR: ") == false)
			{
				return(false);
			}

		} else if(!(level & UNDECOR) && (level & ERROR) && (logger_mode & (TESTING | VERBOSE))){
			// Print out the word "ERROR:"
			if(logger_line_append(line,line_len,"ERROR:") == false)
			{
				return(false);
			}
		}

		if(level & ERROR && logger_mode & ERROR)
		{
			// Print out other arguments
			payload_is_visible = true;

		} else if(level & (REGULAR|ERROR) && logger_mode & REGULAR){
			// Print out other arguments
			payload_is_visible = true;

		} else if(level & (VERBOSE|ERROR) && logger_mode & VERBOSE){
			// Print out other arguments
			payload_is_visible = true;

		} else if(level & (TESTING|ERROR) && logger_mode & TESTING){
			// Print out other arguments
			payload_is_visible = true;
		}
	}

	if(payload_is_visible == true)
	{
		errno = format_errno;

		if(logger_line_append_va(line,line_len,fmt,args) == false)
		{
			return(false);
		}

		errno = format_errno;
	}

	return(true);
}

/**
 * @brief Build and print a formatted log line with file, line, and function metadata
 *
 * @details Literal markers are replaced after printf formatting and sanitization.
 *          Colored REMEMBER messages use a plain copy for the callback, delivered
 *          before terminal output. Other messages are transformed in place.
 *          Allocation or replacement failures suppress the entire message
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
	const int format_errno = errno;
	char *logger_line_text = NULL;
	char *remember_line_text = NULL;
	int line_len = 0;

	va_list args;
	va_start(args,fmt);
	bool line_is_complete = logger_line(&logger_line_text,
		&line_len,
		level,
		filename,
		line,
		funcname,
		fmt,
		args,
		format_errno);
	va_end(args);

	if(line_is_complete == false)
	{
		free(logger_line_text);
		logger_line_text = NULL;
		line_len = 0;
	}

	logger_line_sanitize_for_terminal(&logger_line_text,&line_len);

	if(logger_line_text != NULL)
	{
		size_t output_length = (size_t)line_len;
		size_t remember_length = output_length;
		const bool remember_is_requested = (level & REMEMBER) && rational_remember;

		if(strstr(logger_line_text,"@{") != NULL)
		{
			const bool styles_enabled = rational_color_is_enabled(stdout);

			if(remember_is_requested == true && styles_enabled == true)
			{
				remember_line_text = strdup(logger_line_text);
				line_is_complete = remember_line_text != NULL;

				if(line_is_complete == true)
				{
					line_is_complete = rational_logger_markup_replace(
						&remember_line_text,&remember_length,false);
				}
			}

			if(line_is_complete == true)
			{
				line_is_complete = rational_logger_markup_replace(
					&logger_line_text,&output_length,styles_enabled);
			}
		}

		if(line_is_complete == true)
		{
			if(remember_is_requested == true)
			{
				if(remember_line_text != NULL && remember_length > 0U)
				{
					rational_remember(remember_line_text,(int)remember_length);
				} else if(remember_line_text == NULL && output_length > 0U){
					rational_remember(logger_line_text,(int)output_length);
				}
			}

			flockfile(stdout);
			(void)fwrite(logger_line_text,sizeof(char),output_length,stdout);
			funlockfile(stdout);
		}
	}

	free(remember_line_text);
	free(logger_line_text);
	errno = format_errno;
}

#ifdef TEST
/**
 * @brief Display the plain text received by the demonstration's REMEMBER callback
 *
 * @details The callback prints the supplied text directly, without calling slog().
 *          Its output precedes the logger's styled output of the same message
 *
 * @param[in] message Plain message supplied by the logger
 * @param message_length Number of message bytes to print
 */
void rational_remember(
	const char *message,
	const int  message_length)
{
	/* Show the callback's exact payload without applying formatting or markup */
	printf("REMEMBER callback (plain): ");
	(void)fwrite(message,sizeof(char),(size_t)message_length,stdout);
}

/**
 * @brief Demonstrate logger modes, text styling, sanitization, and REMEMBER output
 *
 * @details Prints labeled examples for visual inspection. Automatic styling uses
 *          the current output stream and environment. Explicit always examples
 *          emit terminal sequences even when output is redirected
 *
 * @return 0 after printing the demonstration
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

	/* Compare the same message under each color policy without changing the
	   environment. Only auto depends on stdout, NO_COLOR, and TERM */
	rational_logger_mode = REGULAR;
	printf("\nColor modes: auto follows stdout, NO_COLOR, and TERM\n");
	printf("The always examples emit ANSI sequences even when output is redirected\n");
	rational_color_mode = COLOR_MODE_AUTO;
	printf("44. auto: ");
	slog(REGULAR|UNDECOR,"@{bold}@{red}styled text@{reset}, plain text\n");
	rational_color_mode = COLOR_MODE_ALWAYS;
	printf("45. always: ");
	slog(REGULAR|UNDECOR,"@{bold}@{red}styled text@{reset}, plain text\n");
	rational_color_mode = COLOR_MODE_NEVER;
	printf("46. never: ");
	slog(REGULAR|UNDECOR,"@{bold}@{red}styled text@{reset}, plain text\n");

	/* Force styling for the palette so every supported color can be inspected.
	   Each styled fragment supplies its own explicit reset marker */
	rational_color_mode = COLOR_MODE_ALWAYS;
	printf("\nColors and text styles (always)\n");
	slog(REGULAR|UNDECOR,"47. Colors: "
		"@{black}black@{reset} @{gray}gray@{reset} @{red}red@{reset} "
		"@{green}green@{reset} @{yellow}yellow@{reset} @{blue}blue@{reset} "
		"@{magenta}magenta@{reset} @{cyan}cyan@{reset} @{white}white@{reset}\n");
	slog(REGULAR|UNDECOR,"48. Weight: @{bold}bold@{reset}, normal after reset\n");
	slog(REGULAR|UNDECOR,"49. Bold colors: "
		"@{boldblack}black@{reset} @{boldred}red@{reset} "
		"@{boldgreen}green@{reset} @{boldyellow}yellow@{reset} "
		"@{boldblue}blue@{reset} @{boldmagenta}magenta@{reset} "
		"@{boldcyan}cyan@{reset} @{boldwhite}white@{reset}\n");
	slog(REGULAR|UNDECOR,"50. Combined markers: @{bold}@{red}bold red@{reset}, plain text\n");

	/* Markers are interpreted after printf arguments have been formatted.
	   Unknown markers are ordinary text and remain visible */
	printf("\nFormatted arguments and unknown markers (always)\n");
	slog(REGULAR|UNDECOR,"51. Markup inside %%s: %s; number: %d\n",
		"@{green}green argument@{reset}",7);
	slog(REGULAR|UNDECOR,"52. Unknown marker stays visible: @{unknown}\n");

	/* Sanitization preserves ordinary UTF-8 and makes raw ESC bytes visible.
	   Raw terminal sequences from arguments are not trusted styling markers */
	printf("\nTerminal-safe text (always)\n");
	slog(REGULAR|UNDECOR,"53. UTF-8 stays readable: Привет 日本語 😀\n");
	slog(REGULAR|UNDECOR,"54. Raw ESC stays visible: %s\n",
		"\033[31mnot colored\033[0m");

	/* The demonstration callback prints the plain message before the logger
	   writes its styled version, making both forms visible one after the other */
	printf("\nREMEMBER (always): plain callback output, then styled logger output\n");
	slog(REGULAR|UNDECOR|REMEMBER,"55. @{bold}@{green}Remembered message %d@{reset}\n",7);

	return 0;
}
#endif
