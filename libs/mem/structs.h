#pragma once

/**
 * @file mem_struct.h
 * @brief Provides memory management header with structures
 *
 */

/// Function exit status
/// Formatted as an enumeration
typedef enum
{
	/// Successfully
	SUCCESS, // The actual value is 0

	/// Unsuccessful
	FAILURE, // The actual value is 1

	// Warning
	WARNING, // The actual value is 2

} Return;
