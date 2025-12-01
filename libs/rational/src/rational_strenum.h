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
	// OK for main() exit status
	GRACEFUL  = 0x0000, // 0000 0000 0000 0000

	/// Successfull
	FAILURE   = 0x0001, // 0000 0000 0000 0001

	/// The process has been permanently stopped
	HALTED    = 0x0002, // 0000 0000 0000 0010

	/// Warning
	WARNING   = 0x0004, // 0000 0000 0000 0100

	/// Do nothing
	DONOTHING = 0x0008, // 0000 0000 0000 1000

	/// Graceful outcome
	SUCCESS  = HALTED | WARNING | DONOTHING  // 0x000E (0000 0000 0000 1110, decimal 14)

#if 0
0x0010 — 0000 0000 0001 0000
0x0020 — 0000 0000 0010 0000
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
