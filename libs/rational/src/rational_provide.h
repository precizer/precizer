#ifndef RATIONAL_PROVIDE_H
#define RATIONAL_PROVIDE_H

#include "rational_enumerations.h"

void provide(const Return);
void deliver(const Return);
const char *show_status(const Return);
Return rational_normalize_return(Return);
bool rational_ask(
	Return *,
	Return,
	const char *,
	const int);

/**
 * @def deliver(status)
 * @brief Finalize and return status like provide(status) but without TRACE output
 *
 * Intended for function exit paths where global/local status reconciliation
 * is required, but `slog(TRACE,...)` on critical return is not desired
 *
 * Behavior:
 * - Evaluates `status` once
 * - Normalizes local status and global_return_status
 * - Merges only GLOBAL bits from global_return_status into the return value
 * - Returns normalized merged flags without printing TRACE logs
 */
	#define deliver(status) \
		{ \
			/* Evaluate once so contract checks and normalization use the same status */ \
			Return __delivered_status = (status); \
			/* A pending yes/no answer must be consumed with ask() before function exit */ \
			if(AWAITING & __delivered_status) \
			{ \
				slog(ERROR,"Unhandled yes/no Return answer before deliver()\n"); \
				return(FAILURE); \
			} \
			/* Normalize, merge global context, and mark fresh yes/no answers as pending */ \
			Return __returned_status = rational_normalize_return(__delivered_status); \
			if(BOOLEAN & __returned_status) \
			{ \
				__returned_status |= AWAITING; \
			} \
			return(__returned_status); \
		}

/**
 * @def provide(status)
 * @brief Finalize and return a status, reconciling it with `global_return_status`.
 *
 * Intended for function exit paths. The macro normalizes conflicting flags and
 * merges local and global state before returning:
 * - Local status is normalized
 * - `global_return_status` is normalized and stored back
 * - Only GLOBAL bits from `global_return_status` are merged into the result
 * - The final result is normalized before return
 *
 * Critical returns also emit a TRACE log record.
 */
	#define provide(status) \
		{ \
			/* Evaluate once so contract checks and normalization use the same status */ \
			Return __provided_status = (status); \
			/* A pending yes/no answer must be consumed with ask() before function exit */ \
			if(AWAITING & __provided_status) \
			{ \
				slog(ERROR,"Unhandled yes/no Return answer before provide()\n"); \
				return(FAILURE); \
			} \
			/* Normalize, merge global context, and mark fresh yes/no answers as pending */ \
			Return __returned_status = rational_normalize_return(__provided_status); \
			if(BOOLEAN & __returned_status) \
			{ \
				__returned_status |= AWAITING; \
			} \
			/* Critical final status is traced by provide(), but not by deliver() */ \
			if(CRITICAL & __returned_status) \
			{ \
				slog(TRACE,"Returned %s:%d status: %s\n",__func__,__LINE__,show_status(__returned_status)); \
			} \
			return(__returned_status); \
		}

/**
 * @def run(func)
 * @brief Conditionally execute a function and merge its flags into `status`.
 * @param func Expression that returns `Return`.
 *
 * The call is performed only when `status` has no `SKIP` bits.
 * This macro expects a writable `Return status` variable in scope.
 *
 * Merge rules:
 * - The callee result is OR-merged into `status`
 * - The accumulated status is normalized after the callee returns
 */
	#define run(func) \
		{ \
			/* A previous yes/no answer must be handled before the next regular call */ \
			if(AWAITING & status) \
			{ \
				slog(ERROR,"Unhandled yes/no Return answer before run()\n"); \
				status = FAILURE; \
			} \
			/* Execute only when current status does not request skipping */ \
			if((SKIP & status) == 0) \
			{ \
				/* Evaluate callee once and capture its returned flag set */ \
				Return __returned_status = (func); \
				/* A yes/no function must be consumed through ask(), not through run() */ \
				if(AWAITING & __returned_status) \
				{ \
					slog(ERROR,"run() received an unhandled yes/no Return answer. Use ask()\n"); \
					status = FAILURE; \
				} else { \
					/* Merge returned flags into current status, then normalize accumulated flags */ \
					status = rational_normalize_return(status | __returned_status); \
				} \
			} \
		}

/**
 * @def call(func)
 * @brief Always execute a function and merge its flags into `status`.
 * @param func Expression that returns `Return`.
 *
 * Unlike `run(func)`, this macro does not check `SKIP`.
 * It expects a writable `Return status` variable in scope.
 *
 * Merge rules:
 * - The callee result is OR-merged into `status`.
 * - The accumulated status is normalized after the callee returns
 */
	#define call(func) \
		{ \
			/* A previous yes/no answer must be handled before the next regular call */ \
			if(AWAITING & status) \
			{ \
				slog(ERROR,"Unhandled yes/no Return answer before call()\n"); \
				status = FAILURE; \
			} else { \
				/* Always evaluate the callee and capture its returned flag set */ \
				Return __returned_status = (func); \
				/* A yes/no function must be consumed through ask(), not through call() */ \
				if(AWAITING & __returned_status) \
				{ \
					slog(ERROR,"call() received an unhandled yes/no Return answer. Use ask()\n"); \
					status = FAILURE; \
				} else { \
					/* Merge returned flags into current status, then normalize accumulated flags */ \
					status = rational_normalize_return(status | __returned_status); \
				} \
			} \
		}

/**
 * @def ask(expr)
 * @brief Consume a yes/no Return expression and return a regular C bool.
 * @param expr Expression that returns `Return`.
 *
 * The expression is evaluated only when local `status` has no `SKIP` bits, or
 * when local `status` already contains a pending yes/no answer to consume.
 * This macro expects a writable `Return status` variable in scope.
 */
#define ask(expr) \
	( \
		(((SKIP & status) == 0) || (AWAITING & status)) \
		&& rational_ask(&status,(expr),__func__,__LINE__) \
	)

#endif // RATIONAL_PROVIDE_H
