
void provide(const Return);
void deliver(const Return);
const char *show_status(const Return);
Return normalize_return_status(Return);

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
		/* Evaluate, normalize, merge global context, and return once */ \
		return(normalize_return_status(status)); \
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
		/* Evaluate, normalize, and merge global context once */ \
		Return __returned_status = normalize_return_status(status); \
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
		/* Execute only when current status does not request skipping */ \
		if((SKIP & status) == 0) \
		{ \
			/* Evaluate callee once and capture its returned flag set */ \
			Return __returned_status = (func); \
			/* Merge returned flags into current status, then normalize accumulated flags */ \
			status = normalize_return_status(status | __returned_status); \
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
		/* Always evaluate the callee and capture its returned flag set */ \
		Return __returned_status = (func); \
		/* Merge returned flags into current status, then normalize accumulated flags */ \
		status = normalize_return_status(status | __returned_status); \
	}
