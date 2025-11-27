/**
 *
 * @file
 * @brief Functions and structs to convert a number of bytes into a human-readable string
 *
 */

/// Structure to record the number of bytes.
typedef struct {
	size_t bytes;
	size_t kibibytes;
	size_t mebibytes;
	size_t gibibytes;
	size_t tebibytes;
	size_t pebibytes;
	size_t exbibytes;
} Byte;

char *bkbmbgbtbpbeb(const size_t);
