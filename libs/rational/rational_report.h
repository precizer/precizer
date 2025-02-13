/**
 *
 * @file rational_report.h
 * @brief Prototypes of functions for report an error without relying on dynamic memory
 *
 */

// Safe ERror Print
void SERP(
	const char *,
	const char *,
	const char *
);

/* Macro wrapper for convenient usage */
#define serp(prefix) SERP(prefix,__FILE__,__func__)

void REPORT(
	const char *,
	const char *,
	int,
	const char *,
	...
);

// Convenience macro to automatically include source file, function, and line information
#define report(...) REPORT(__FILE__,__func__,__LINE__,__VA_ARGS__)
