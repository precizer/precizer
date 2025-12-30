/**
 *
 * @file
 * @brief Prototypes of functions and macros for logging
 *
 */

#include <stdarg.h>

/* Atomic operations */
#include <stdatomic.h>

// Global flag to manage output of all logging messages
// in an application
extern _Atomic char rational_logger_mode;
extern _Atomic Return global_return_status;

/**
 *
 * When print a message
 * REGULAR — unconditional printf()
 * VERBOSE — printf() only when verbose mode has been determined.
 * TESTING — for testing purposes
 * ERROR   — error message only. Will be shown when any of the above modes are engaged
 * UNDECOR — suppress logging prefixes (time/file/line/func and mode labels) for this call
 *           (abbreviation of "UNDECORATED")
 * SILENT  — disable all output
 *
 */
typedef enum
{
	REGULAR = 0x01, // 000001
	VERBOSE = 0x02, // 000010
	TESTING = 0x04, // 000100
	TRACE   = 0x06, // 000110 = VERBOSE|TESTING
	EVERY   = 0x07, // 000111 = REGULAR|VERBOSE|TESTING
	ERROR   = 0x08, // 001000
	SILENT  = 0x10, // 010000
	UNDECOR = 0x20  // 100000

} LOGMODES;

char *rational_reconvert(int);

/**
 * @brief Converts a macro name to a string representation
 * @param x The macro name to be converted
 * @return A pointer to a constant char string containing the macro name
 */
#define rational_convert(x) #x

// The definition creates a shorthand for logging messages with additional
// context information, such as the file name, line number, and function name
#define slog(x,...) rational_logger(x,__FILE__,__LINE__,__func__,__VA_ARGS__ )

void rational_logger
(
	const char,
	const char *,
	size_t,
	const char *,
	const char *,
	...);
