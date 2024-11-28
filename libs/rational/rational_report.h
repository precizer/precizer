/**
 *
 * @file rational_report.h
 * @brief Prototypes of functions for report an error without relying on dynamic memory
 *
 */

// Convenience macro to automatically include source file, function, and line information
#define report(...) REPORT(__FILE__, __func__, __LINE__, __VA_ARGS__)

void REPORT(
	const char *,
	const char *,
	int,
	const char *,
	...
);
