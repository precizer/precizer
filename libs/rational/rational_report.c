#include "rational.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Reports an error message to the specified file descriptor.
 * 
 * This function generates an error message describing the failed memory
 * reallocation attempt and writes it directly to the file descriptor
 * without relying on dynamic memory allocation.
 *
 * @param fd File descriptor to which the error message will be written.
 * @param msg General description of the error context.
 * @param failed_size The size (in bytes) for which memory reallocation failed.
 */
void report(const char *msg, size_t failed_size)
{
	char buffer[256];  ///< Buffer for storing the formatted error message.

	// Format the error message with details about the failed operation.
	// snprintf is used for safety to ensure buffer overflow is avoided.
	int len = snprintf(buffer,
				sizeof(buffer),
				"ERROR: %s for %zu bytes: %s (errno: %d)\n",
				msg,
				failed_size,
				strerror(errno),
				errno
	);

	// Write the message to the file descriptor.
	// The length is limited to the smaller of `len` or buffer size to avoid overflows.
	if (len > 0) {
		write(STDERR_FILENO, buffer, (len < (int)sizeof(buffer)) ? len : sizeof(buffer));
	}
}

#if 0
/// Test

// Example of usage

int main(void){

	size_t new_aligned_bytes = 100ULL;

	// Report an error without relying on dynamic memory
	report("Memory reallocation failed", new_aligned_bytes);

	return 0; // Success
}
#endif
