#include "precizer.h"

/**
 * If the SHA512 checksum calculation is interrupted,
 * the function outputs a message indicating the last
 * successfully processed byte
 *
 */
void show_checksum_gracefully_interrupted(
	const char          *relative_path,
	const sqlite3_int64 *offset
){
	if(*offset > 0 && global_interrupt_flag == true)
	{
		/* Truncate the file path/name in the display output if it exceeds the length limit */
		char *shorten_relative_path = strdup(relative_path);

		(void)shorten_path(shorten_relative_path);

		slog(EVERY,"SHA512 checksum for the file %s has been gracefully interrupted at byte: %s\n",shorten_relative_path,bkbmbgbtbpbeb((size_t)*offset));

		free(shorten_relative_path);
	}
}
