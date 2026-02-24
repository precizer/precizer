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
typedef enum : unsigned int
{
	/// Excellent, great, fine, valuable,
	/// proper, graceful, successful
	OK = 0x0000u,        // 0000 0000 0000 0000

	/// Boolean value: `false`
	NO = 0x0000u,        // 0000 0000 0000 0000

	// Shell exit status
	COMPLETED = 0x0000u, // 0000 0000 0000 0000

	/// Fail, Internal fail
	FAILURE = 0x0001u,   // 0000 0000 0000 0001

	/// Boolean value: `true`
	YES = 0x0002u,       // 0000 0000 0100 0000

	/// Unsuccessful
	UNSUCCESS = 0x0004u, // 0000 0000 0000 0100

	/// Successful
	SUCCESS = 0x0008u,   // 0000 0000 0000 1000

	/// The process has been permanently stopped
	HALTED = 0x0010u,    // 0000 0000 0001 0000

	/// Warning
	WARNING = 0x0020u,   // 0000 0000 0010 0000

	/// Do nothing
	DONOTHING = 0x0040u, // 0000 0000 0100 0000

	/// Informational
	INFO = 0x0080u,      // 0000 0000 1000 0000

	/// Critical set
	CRITICAL = WARNING | UNSUCCESS | FAILURE,  // Hex: 0x0026. Dec: 38, Bin: 0000 0000 0010 0110

	/// Graceful outcome
	TRIUMPH = SUCCESS | HALTED | DONOTHING | INFO,   // Hex: 0x00D8, Dec: 216, Bin: 0000 0000 1101 1000

	/// Skip the normal function call based on the status flag
	SKIP = INFO | WARNING | UNSUCCESS | FAILURE | HALTED      // Hex: 0x00B5. Dec: 181, Bin: 0000 0000 1011 0101

#if 0
/*

   // The next flag values
   0x0100 - 0000 0001 0000 0000
   0x0200 - 0000 0010 0000 0000
   0x0400 - 0000 0100 0000 0000
   0x0800 - 0000 1000 0000 0000
   0x1000 - 0001 0000 0000 0000
   0x2000 - 0010 0000 0000 0000
   0x4000 - 0100 0000 0000 0000
   0x8000 - 1000 0000 0000 0000

   // Possible flag names
   NO ↔ YES
   NONE ↔ ANY (or sometimes SOME)
   NONE ↔ ALL (only when the intended contrast is between none and all)
   NULL ↔ VALID (or NONNULL) for pointers
   EMPTY ↔ NONEMPTY (or HASITEMS)
   DISABLED ↔ ENABLED
   OFF ↔ ON
   FALSE ↔ TRUE
   FAIL / FAILURE ↔ SUCCESS (or OK)
   INVALID ↔ VALID
   DENY / DENIED ↔ ALLOW / ALLOWED
   NOTFOUND ↔ FOUND
   STOPPED / HALTED ↔ RUNNING (or ACTIVE)

 */
#endif

} Return;
