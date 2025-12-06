#include "precizer.h"

/**
 * If the SHA512 checksum calculation is interrupted,
 * the function outputs a message indicating the last
 * successfully processed byte
 *
 */
void show_checksum_gracefully_interrupted(
	const char          *relative_path,
	const sqlite3_int64 *offset)
{
	if(*offset > 0 && global_interrupt_flag == true)
	{
		slog(EVERY,"SHA512 checksum for the file %s has been gracefully interrupted at byte: %s\n",relative_path,bkbmbgbtbpbeb((size_t)*offset));
	}
}
