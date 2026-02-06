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
	/// Excellent, great, fine, valuable, proper, graceful
	COMPLETED = 0x0000, // 0000 0000 0000 0000

	/// Unsuccessful
	FAILURE   = 0x0001, // 0000 0000 0000 0001

	/// Successful
	SUCCESS   = 0x0002, // 0000 0000 0000 0010

	/// The process has been permanently stopped
	HALTED    = 0x0004, // 0000 0000 0000 0100

	/// Warning
	WARNING   = 0x0008, // 0000 0000 0000 1000

	/// Do nothing
	DONOTHING = 0x0010, // 0000 0000 0001 0000

	/// Critical
	CRITICAL = 0x0020, // 0000 0000 0010 0000

	/// Graceful outcome
	TRIUMPH   = SUCCESS | HALTED | DONOTHING  // 0x0016 (0000 0000 0001 0110, decimal 22)

#if 0
0x0040 — 0000 0000 0100 0000
0x0080 — 0000 0000 1000 0000
0x0100 — 0000 0001 0000 0000
0x0200 — 0000 0010 0000 0000
0x0400 — 0000 0100 0000 0000
0x0800 — 0000 1000 0000 0000
0x1000 — 0001 0000 0000 0000
0x2000 — 0010 0000 0000 0000
0x4000 — 0100 0000 0000 0000
0x8000 — 1000 0000 0000 0000
#endif

} Return;
