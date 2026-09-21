/**
 *
 * @file
 * @brief Prototypes of functions and macros for logging
 *
 */

#ifndef RATIONAL_LOGGER_H
#define RATIONAL_LOGGER_H

#include "rational_enumerations.h"

#include <stdbool.h>
#include <stddef.h>

/* Atomic operations */
#include <stdatomic.h>

// Global flag to manage output of all logging messages
// in an application
typedef enum LOGMODES : unsigned int
{
	REGULAR = 0x01u,           // 0000001
	VERBOSE = 0x02u,           // 0000010
	TESTING = 0x04u,           // 0000100
	TRACE = VERBOSE | TESTING, // Hex: 0x06. Dec: 6. Bin: 0000110
	EVERY = REGULAR | VERBOSE | TESTING,   // Hex: 0x07. Dec: 7. Bin: 0000111
	ERROR = 0x08u,             // 0001000
	SILENT = 0x10u,            // 0010000
	UNDECOR = 0x20u,           // 0100000
	REMEMBER = 0x40u,          // 1000000
	VISIBLE_IN_SILENT = 0x80u  // 10000000
} LOGMODES;

extern _Atomic LOGMODES rational_logger_mode;

/**
 *
 * When print a message
 * REGULAR — unconditional printf()
 * VERBOSE — printf() only when verbose mode has been determined.
 * TESTING — for testing purposes
 * ERROR   — error message only. Will be shown when any of the above modes are engaged
 * UNDECOR — suppress logging prefixes (time/file/line/func and mode labels) for this call
 *           (abbreviation of "UNDECORATED")
 * REMEMBER — pass the formatted log line to optional rational_remember() callback
 * SILENT  — disable all output
 * VISIBLE_IN_SILENT — when SILENT is active, still print the payload for this call without logger prefixes
 *
 */
char *rational_reconvert(LOGMODES);

/**
 * @brief Converts a macro name to a string representation
 * @param x The macro name to be converted
 * @return A pointer to a constant char string containing the macro name
 */
#define rational_convert(x) #x

// The definition creates a shorthand for logging messages with additional
// context information, such as the file name, line number, and function name
#define slog(x,...) rational_logger(x,__FILE__,__LINE__,__func__,__VA_ARGS__ )

/*
 * Optional callback contract for REMEMBER logs
 *
 * If the main program defines:
 *   void rational_remember(const char *message, int message_length);
 * then any slog() call with REMEMBER will pass the fully formatted plain log
 * line and its byte length to this function, including any trailing line
 * terminator. Terminal decorations affect immediate terminal output only;
 * this callback never receives terminal control sequences
 *
 * If the program does not define it, the logger's weak reference resolves to
 * NULL and the call is skipped
 *
 * The message pointer is valid only during the call; copy it if needed.
 * Avoid calling slog() inside rational_remember() to prevent recursion
 */
void rational_remember(
	const char *,
	const int);

void rational_logger
(
	const LOGMODES,
	const char *,
	size_t,
	const char *,
	const char *,
	...);

bool rational_logger_replace_all(
	char **,
	size_t *,
	const char *,
	const char *);

bool rational_logger_markup_replace(
	char **,
	size_t *,
	bool);
#endif // RATIONAL_LOGGER_H
