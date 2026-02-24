
void provide(const Return);
void deliver(const Return);
const char *show_status(const Return);

/**
 * @def deliver(status)
 * @brief Finalize and return status like provide(status) but without TRACE output
 *
 * Intended for function exit paths where global/local status reconciliation
 * is required, but `slog(TRACE,...)` on critical return is not desired
 *
 * Behavior:
 * - Evaluates `status` once
 * - Clears `SUCCESS` in `global_return_status` if global state is `CRITICAL`
 * - If local status is `CRITICAL`, merges global non-`TRIUMPH` bits
 * - If local status is `TRIUMPH`, merges global non-`CRITICAL` bits
 * - Returns merged flags without printing TRACE logs
 */
#define deliver(status) \
	{ \
		/* Evaluate the input once and work on a local mutable copy */ \
		Return __returned_status = status; \
		/* If global state is already critical, drop SUCCESS to avoid contradictory flags */ \
		if(CRITICAL & global_return_status) \
		{ \
			/* Keep all global bits except SUCCESS */ \
			global_return_status &= ~SUCCESS; \
		} \
		/* Critical local exit path: prioritize critical global context over graceful global bits */ \
		if(CRITICAL & __returned_status) \
		{ \
			/* Merge all global non-TRIUMPH bits (e.g., FAILURE/WARNING/UNSUCCESS) */ \
			__returned_status |= (global_return_status & ~TRIUMPH); \
		} else if(TRIUMPH & __returned_status){ \
			/* Graceful local exit path: keep global graceful context, suppress global critical bits */ \
			__returned_status |= (global_return_status & ~CRITICAL); \
		} \
		/* Return the normalized and merged status flags */ \
		return(__returned_status); \
	}

/**
 * @def provide(status)
 * @brief Finalize and return a status, reconciling it with `global_return_status`.
 *
 * Intended for function exit paths. The macro normalizes conflicting flags and
 * merges local and global state before returning:
 * - If `global_return_status` is already critical, `SUCCESS` is removed from it.
 * - If local `status` contains any `CRITICAL` bit, all non-`TRIUMPH` global bits
 *   are merged into the return value.
 * - If local `status` contains any `TRIUMPH` bit, all non-`CRITICAL` global bits
 *   are merged into the return value.
 *
 * Critical returns also emit a TRACE log record.
 */
#define provide(status) \
	{ \
		/* Evaluate the input once and work on a local mutable copy */ \
		Return __returned_status = status; \
		/* If global state is already critical, drop SUCCESS to avoid contradictory flags */ \
		if(CRITICAL & global_return_status) \
		{ \
			/* Keep all global bits except SUCCESS */ \
			global_return_status &= ~SUCCESS; \
		} \
		/* Critical local exit path: prioritize critical global context over graceful global bits */ \
		if(CRITICAL & __returned_status) \
		{ \
			/* Merge all global non-TRIUMPH bits (e.g., FAILURE/WARNING/UNSUCCESS) */ \
			__returned_status |= (global_return_status & ~TRIUMPH); \
			/* Emit trace for final critical return composition */ \
			slog(TRACE,"Returned %s:%d status: %s\n",__func__,__LINE__,show_status(__returned_status)); \
		} else if(TRIUMPH & __returned_status){ \
			/* Graceful local exit path: keep global graceful context, suppress global critical bits */ \
			__returned_status |= (global_return_status & ~CRITICAL); \
		} \
		/* Return the normalized and merged status flags */ \
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
 * - If the callee returns `CRITICAL`, `SUCCESS` is removed from both the callee
 *   result and current `status`.
 * - The callee result is then OR-merged into `status`
 */
#define run(func) \
	{ \
		/* Execute only when current status does not request skipping */ \
		if((SKIP & status) == false) \
		{ \
			/* Evaluate callee once and capture its returned flag set */ \
			Return __returned_status = (func); \
			/* Detect any critical bit returned by the callee */ \
			if(CRITICAL & __returned_status) \
			{ \
				/* Remove SUCCESS from callee status to avoid SUCCESS|CRITICAL combination */ \
				__returned_status &= ~SUCCESS; \
				/* Remove SUCCESS from accumulated status for the same reason */ \
				status &= ~SUCCESS; \
			} \
			/* Merge returned flags into current accumulated status */ \
			status |= __returned_status; \
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
 * - If the callee returns `CRITICAL`, `SUCCESS` is removed from both the callee
 *   result and current `status`.
 * - If current `status` is already critical, `SUCCESS` is removed from the callee
 *   result before merge.
 * - The callee result is then OR-merged into `status`.
 */
#define call(func) \
	{ \
		/* Always evaluate the callee and capture its returned flag set */ \
		Return __returned_status = (func); \
		/* Detect any critical bit returned by the callee */ \
		if(CRITICAL & __returned_status) \
		{ \
			/* Remove SUCCESS from callee status to avoid SUCCESS|CRITICAL combination */ \
			__returned_status &= ~SUCCESS; \
			/* Remove SUCCESS from accumulated status for the same reason */ \
			status &= ~SUCCESS; \
		} \
		/* If accumulated status is already critical, keep callee non-successful */ \
		if(CRITICAL & status) \
		{ \
			/* Remove SUCCESS from callee status before merging */ \
			__returned_status &= ~SUCCESS; \
		} \
		/* Merge returned flags into current accumulated status */ \
		status |= __returned_status; \
	}
