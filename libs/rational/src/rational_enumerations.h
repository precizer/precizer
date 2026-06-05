/**
 * @file
 * @brief Common usage enumerations and bit flags
 *
 */
#ifndef RATIONAL_ENUMERATIONS_H
#define RATIONAL_ENUMERATIONS_H

/*
 *
 * Common flag type definitions
 *
 */

/// Function exit status
/// Formatted as fixed-underlying unsigned bit flags
///
/// Return flags are grouped into independent layers:
/// - Technical status reports whether the function itself completed safely
/// - Binary status carries local yes/no answers from check functions
/// - Context status carries process-level or flow-control information
typedef enum Return : unsigned int
{
	/// Excellent, great, fine, valuable,
	/// proper, graceful, successful
	OK = 0x0000u,        // 0000 0000 0000 0000

	// Shell exit status
	COMPLETED = 0x0000u, // 0000 0000 0000 0000

	/// Internal failure
	FAILURE = 0x0001u,   // 0000 0000 0000 0001

	/// Function completed without an internal failure
	SUCCESS = 0x0008u,   // 0000 0000 0000 1000

	/// Process flow was permanently stopped
	HALTED = 0x0010u,    // 0000 0000 0001 0000

	/// Non-fatal warning
	WARNING = 0x0020u,   // 0000 0000 0010 0000

	/// Skip the requested action without treating it as a failure
	DONOTHING = 0x0040u, // 0000 0000 0100 0000

	/// Informational result
	INFO = 0x0080u,      // 0000 0000 1000 0000

	/// Positive local answer from a check function
	YES = 0x0100u,      // 0000 0001 0000 0000

	/// Negative local answer from a check function
	NO = 0x0200u,       // 0000 0010 0000 0000

	/// Internal marker for an unhandled local yes/no answer
	AWAITING = 0x0400u, // 0000 0100 0000 0000

	/// Technical status bits that report internal or blocking problems
	// Hex: 0x0021. Dec: 33. Bin: 0000 0000 0010 0001
	CRITICAL = WARNING | FAILURE,

	/// Technical and flow bits that report graceful outcomes
	// Hex: 0x00D8. Dec: 216. Bin: 0000 0000 1101 1000
	TRIUMPH = SUCCESS | HALTED | DONOTHING | INFO,

	/// Status bits allowed to propagate from global_return_status
	// Hex: 0x00B0. Dec: 176. Bin: 0000 0000 1011 0000
	GLOBAL = INFO | WARNING | HALTED,

	/// Local binary answer bits for caller-side decisions
	// Hex: 0x0300. Dec: 768. Bin: 0000 0011 0000 0000
	BOOLEAN = YES | NO,

	/// Skip the normal function call based on the status flag
	// Hex: 0x0031. Dec: 49. Bin: 0000 0000 0011 0001
	SKIP = WARNING | FAILURE | HALTED

} Return;

#if 0
/*

   // The next flag values
   0x0800 - 0000 1000 0000 0000
   0x1000 - 0001 0000 0000 0000
   0x2000 - 0010 0000 0000 0000
   0x4000 - 0100 0000 0000 0000
   0x8000 - 1000 0000 0000 0000

   // Possible flag names
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

/*
 * File metadata change bits
 *
 */
typedef enum Changed : unsigned int
{
	IDENTICAL = 0x00u,                 // 000000
	NOT_EQUAL = 0x01u,                 // 000001
	SIZE_CHANGED = 0x02u,              // 000010
	STATUS_CHANGED_TIME = 0x04u,       // 000100
	MODIFICATION_TIME_CHANGED = 0x08u, // 001000
	ALLOCATED_SIZE_CHANGED = 0x10u,    // 010000
	COMPARE_FAILED = 0x20u             // 100000

} Changed;

#endif // RATIONAL_ENUMERATIONS_H
