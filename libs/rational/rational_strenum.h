/**
 * @file
 * @brief Common usage structures and enumerations
 *
 */

/*
 *
 * Initialization of enumerations
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

	// Do nothing
	DONOTHING // The actual value is 3

} Return;
