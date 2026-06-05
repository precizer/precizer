[Ссылка на русскоязычную страницу README](README.ru.md)

# librational — shared return statuses for C code

`librational` is a small internal library with shared flags, return macros, logging, value formatting, and helper functions for C code. Its most visible part is the `Return` type, which lets a function return several independent signals in one value instead of one flat numeric code.

Besides `Return` handling, the library can write messages through a shared report/logger layer, read the current time, format numbers with thousands separators, convert byte counts into human-readable text, and convert nanosecond durations into strings with years, months, weeks, days, hours, and smaller units.

This return style is useful when code needs to know separately:

* whether a technical error happened inside the function;
* whether the function completed normally;
* what logical answer the function produced, if it checked something;
* whether a global process context should affect later returns.

## Contents

1. [Core idea](#core-idea)
2. [Quick start](#quick-start)
3. [Formatting helpers](#formatting-helpers)
4. [Time helpers](#time-helpers)
5. [Status text helpers](#status-text-helpers)
6. [Reports and logging](#reports-and-logging)
7. [Status layers](#status-layers)
8. [Basic function](#basic-function)
9. [Check function with YES and NO](#check-function-with-yes-and-no)
10. [Reading the result correctly](#reading-the-result-correctly)
11. [Sequential Checks Through status](#sequential-checks-through-status)
12. [Call chains: run() and call()](#call-chains-run-and-call)
13. [Global status](#global-status)
14. [Technical Details](#technical-details)
15. [Practical rules](#practical-rules)

## Core idea

A plain `int` return code often mixes different meanings. For example, `access()` returns `0` when a file is accessible and `-1` when it is not accessible or when an error happened. That is enough for a small API, but in a larger application it is useful to distinguish:

* the function itself worked correctly, but the check produced a negative answer;
* the function hit an internal technical problem;
* the program is already in a global stopped or warning state.

`Return` handles this with bit flags. One returned value can look like this:

```c
SUCCESS | YES
```

It means: the function completed without an internal error, and the logical check result is positive.

Another example:

```c
SUCCESS | NO
```

It means: the function completed without an internal error, but the logical check result is negative. For example, a file is not accessible, a record was not found, or a condition is not satisfied.

## Quick start

A regular function starts with local `status` and exits through `provide(status)`.

```c
Return load_settings(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	provide(status);
}
```

A check function returns a regular technical result together with a local `YES` or `NO` answer.

```c
Return path_is_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= YES; /* or NO */

	provide(status);
}
```

Caller code reads that answer through `ask(...)`.

```c
if(ask(path_is_readable(path)))
{
	run(print_file(path));
}
```

Short cheat sheet:

| Need | Use |
|---|---|
| return status from a regular function | `provide(status)` |
| return status without TRACE log | `deliver(status)` |
| run a work step that may be skipped after failure | `run(func())` |
| run cleanup or mandatory final action | `call(func())` |
| read `YES` or `NO` | `ask(check_func())` |
| manually continue only on normal status | `if(TRIUMPH & status)` |
| stop a loop after any `SKIP` status | `if((SKIP & status) == 0)` |

Do not confuse:

* `NO` is not `false` or `0`;
* `NO` is not `FAILURE`;
* `YES` and `NO` are not passed through `run()` and `call()`;
* `YES` and `NO` are not returned upward if the current function is not itself a check function.

## Formatting helpers

`librational` also contains small functions for preparing values for human-readable output. They are useful in logs, reports, test messages, and CLI output, where clear text is more useful than rebuilding the same formatting in caller code each time.

The numeric macro `form(value,buffer,buffer_size)` chooses the matching formatter from the argument type. Real values are formatted with a comma as the thousands separator, a dot as the decimal separator, rounding to at most 9 fractional digits, and removal of trailing fractional zeros. Integer values are formatted in groups of three digits. For example:

```c
char text[FORM_OUTPUT_BUFFER_SIZE];

printf("%s\n",form(1234567.125L,text,sizeof(text))); /* 1,234,567.125 */
printf("%s\n",form((int)-12345,text,sizeof(text)));  /* -12,345 */
```

The reentrant functions `form_real_r()`, `form_intmax_r()`, and `form_uintmax_r()` are available for explicit calls. They write the result into caller-provided storage. If the buffer for `form_real_r()` is too small, the function first reduces fractional precision. If the value still does not fit, it writes an empty string. Integer formatters write an empty string when the complete integer value cannot fit.

The `itoa(value,buffer,base)` function converts an `int` to a string in bases from 2 to 36. In base 10, negative numbers are printed with a leading `-`. In other bases, negative values are printed as an unsigned bit pattern, which is useful for hexadecimal and binary dumps. The buffer must be large enough: the function does not receive its size and cannot protect itself from overflow. For an invalid base, the function writes an empty string, sets `errno = EINVAL`, and returns `buffer`; for a `NULL` buffer, it sets `errno = EINVAL` and returns `NULL`.

```c
char number[33];

printf("%s\n",itoa(255,number,16));  /* FF */
printf("%s\n",itoa(-789,number,10)); /* -789 */
```

The `bkbmbgbtbpbeb()` and `bkbmbgbtbpbeb_r()` functions convert a byte count into a string with binary units: `B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`, and `EiB`. `FULL_VIEW` shows all non-zero units, while `MAJOR_VIEW` keeps only the largest unit:

```c
printf("%s\n",bkbmbgbtbpbeb(1536,FULL_VIEW));  /* 1KiB 512B */
printf("%s\n",bkbmbgbtbpbeb(1536,MAJOR_VIEW)); /* 1KiB */
```

The `form_date()` and `form_date_r()` functions convert a nanosecond duration into a string with time units. `FULL_VIEW` prints all non-zero parts, while `MAJOR_VIEW` prints only the largest part:

```c
printf("%s\n",form_date(3600000000001LL,FULL_VIEW));  /* 1h 1ns */
printf("%s\n",form_date(3600000000001LL,MAJOR_VIEW)); /* 1h */
```

Functions without the `_r` suffix return a pointer to an internal static buffer. This is convenient for short output, but the next call to the same function overwrites the previous string. Functions with the `_r` suffix accept caller-provided storage and are the right choice when several formatted results must live at the same time or when shared static state should be avoided.

## Time helpers

`librational` contains a few small functions for reading time and formatting timestamps. They are useful in logs, tests, and measurements where repeating the same system-clock boilerplate would add noise to caller code.

`cur_time_ms()` returns milliseconds since the Unix epoch from the system wall clock. `cur_time_ns()` returns nanoseconds since the Unix epoch from `CLOCK_REALTIME`. These values are tied to real calendar time and may jump if the system clock is adjusted.

`cur_time_monotonic_ns()` returns nanoseconds from a monotonic clock source when one is available on the target platform. This value is not a calendar timestamp: it is meant for measuring intervals between two events. If a monotonic clock is not available at build time, the name transparently falls back to `cur_time_ns()`.

`seconds_to_ISOdate(seconds)` converts Unix time in seconds to a local-time string shaped as `YYYY-MM-DD HH:MM:SS`. The function returns a pointer to an internal static buffer, so the next call overwrites the previous result. To format the current time, pass `time(NULL)`; the value `0` is the Unix epoch itself, not “now”.

```c
printf("%lld\n",cur_time_ms());
printf("%s\n",seconds_to_ISOdate(time(NULL)));
```

## Status Text Helpers

`show_status(status)` converts a `Return` value to a short string for logs, debug messages, and tests. The zero `OK` status is printed as `OK`, and known flags are joined with `|`, for example `SUCCESS|YES` or `FAILURE|WARNING`. If the value has no known flags, the function returns `UNKNOWN`.

For compound statuses, the function uses an internal static buffer. Copy the string into caller-owned storage if it must survive the next `show_status()` call.

```c
Return status = SUCCESS | YES;
printf("%s\n",show_status(status)); /* SUCCESS|YES */
```

## Reports and Logging

Two low-level report helpers are available for diagnostic messages. `serp(prefix)` prints a message to `stderr` with the current `errno`, source file, and function name. `report(format,...)` prints a formatted error message with source file, function, source line, and decoded `errno`. These helpers are meant for error paths and do not require heap allocation.

```c
errno = EINVAL;
serp("Invalid input");
report("Failed to process item %d",item_id);
```

The main logger is called through `slog(level,format,...)`. The macro automatically adds the source file, line, and function name, while output is controlled by the global atomic `rational_logger_mode`.

Main modes:

* `REGULAR` — regular messages
* `VERBOSE` — detailed messages with timestamp and call site
* `TESTING` — messages for test output
* `ERROR` — error messages
* `SILENT` — suppress regular output
* `UNDECOR` — print only the payload without logger prefixes
* `REMEMBER` — pass the prepared line to the optional `rational_remember()` callback
* `VISIBLE_IN_SILENT` — allow a specific message to appear even in `SILENT`

`rational_reconvert(mode)` returns a human-readable string with mode flag names, for example `REGULAR | VERBOSE`. `rational_convert(NAME)` is a simple macro-stringify helper that turns a macro name into text.

```c
rational_logger_mode = REGULAR | VERBOSE;
slog(REGULAR,"Started\n");
slog(VERBOSE,"Detailed value: %d\n",value);
```

## Status layers

### Technical layer

The technical layer reports what happened to the function itself.

| Flag | Meaning |
|---|---|
| `SUCCESS` | the function completed without an internal error |
| `FAILURE` | a technical error happened inside the function: not enough memory, damaged internal state, or inability to continue |
| `WARNING` | the operation completed, but there is an important problem that caller code should know about |
| `DONOTHING` | the action was deliberately skipped and this is not an error |
| `CRITICAL` | mask of problematic technical flags: `WARNING | FAILURE` |
| `TRIUMPH` | mask of normal and controlled outcomes: `SUCCESS | HALTED | DONOTHING | INFO` |

Use `FAILURE` only for problems that are not normal application logic. Examples: memory allocation failed, an internal structure is in an unexpected state, or a library function received an invalid descriptor.

A status can contain several flags at once, so code usually checks it with bit masks.

```c
if(CRITICAL & status)
{
	/* Technical problem: WARNING or FAILURE is present */
}

if(WARNING & status)
{
	/* WARNING is present, even if other flags are present too */
}
```

### Binary layer

The binary layer is for yes/no answers without mixing them with technical success or technical failure.

| Flag | Meaning |
|---|---|
| `YES` | local answer from a check function: yes, the condition is true |
| `NO` | local answer from a check function: no, the condition is false |
| `BOOLEAN` | binary flag mask: `YES | NO` |

Important: `YES` and `NO` are not C `bool` values. They are bit flags inside `Return`. That means `NO` is not equal to `0`, and code like `if(path_is_readable(path))` is not the correct way to read the answer.

Example: a function checks whether a path is readable. If the check runs normally and the path is readable, it can return `SUCCESS | YES`. If the check runs normally but the path is not readable, it can return `SUCCESS | NO`.

A simple rule: `YES` and `NO` fit functions that ask a question by their meaning. For example: `is_*`, `has_*`, `can_*`, `check_*`, `validate_*`. Such an answer is read through `ask(...)`, which returns a regular C `true` or `false` result.

`YES` and `NO` are not meant to be inherited automatically through a chain of regular functions. Return them upward only when the current function itself also promises a yes/no answer.

### Global layer

The global layer is for process state that can be set outside the current function. For example, a signal handler can set `global_return_status` to `HALTED` so later returns know that the program should stop.

| Flag | Meaning |
|---|---|
| `INFO` | informational result. For example, the application printed `--help`, `--version`, or another help screen and exited normally |
| `WARNING` | warning that should be visible outside one function |
| `HALTED` | process is stopped or should stop. For example, the user pressed Ctrl+C and the signal handler requested shutdown |
| `GLOBAL` | mask of flags that may propagate from `global_return_status` into a regular return |

Only `GLOBAL` bits are propagated from `global_return_status` into a function result. Binary answers `YES` and `NO` do not leak between functions through global status.

## Basic function

A regular function creates a local `status`, changes it while it works, and exits through `provide(status)`.

```c
#include "rational.h"

Return load_settings(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Some internal operation failed.
	   This is a technical failure, not a normal business result */
	if(settings_storage_is_broken())
	{
		status = FAILURE;
	}

	provide(status);
}
```

`provide()` does three things:

* normalizes the local status;
* applies allowed global flags from `global_return_status`;
* writes a TRACE log if the final status contains `CRITICAL`.

If TRACE output is not needed on return, use `deliver(status)`.

```c
Return quiet_cleanup(void)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Cleanup may be called from paths where extra TRACE output is noisy */
	release_temporary_buffers();

	deliver(status);
}
```

## Check function with YES and NO

Binary flags are useful for functions that check a condition. In this example, lack of file access is not treated as an internal function failure. The function was able to perform the check, so the technical layer remains `SUCCESS`.

```c
#include <unistd.h>

#include "rational.h"

Return path_is_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(path == NULL)
	{
		/* NULL input is a caller or program error.
		   The function cannot perform a meaningful check */
		status = FAILURE;
	}

	/* Run the logical check only while the technical status is still successful */
	if(TRIUMPH & status)
	{
		if(access(path,R_OK) == 0)
		{
			/* The check ran normally and the answer is positive */
			status |= YES;
		} else {
			/* The check ran normally and the answer is negative */
			status |= NO;
		}
	}

	provide(status);
}
```

The `if(TRIUMPH & status)` form is important here. It lets code set the technical status first and then run the next step only while earlier steps have not failed. This style helps avoid two common problems:

* cascading `if` inside `if`, where each indentation level makes code harder to read;
* needing `goto` to jump to shared cleanup at the end of a function.

A detailed walkthrough of this pattern is below in [“Sequential Checks Through status”](#sequential-checks-through-status).

## Reading the result correctly

### What a check function returns

A check function returns a regular `Return`. If the check itself completed without a technical failure, `YES` or `NO` is added to `SUCCESS`.

In the example below, `path_is_readable()` returns `YES` or `NO` because its meaning is to answer the question “is this file readable?”. `print_file_if_readable()` no longer answers that question. It only decides whether to print the file, so it does not return `YES` or `NO` further upward.

The main rule: do not read `NO` as a technical failure. It is a negative logical answer. If a check function returns a technical error, `ask(...)` adds that error to the local `status` and returns `false`. If a check function returns a normal `NO`, `ask(...)` also returns `false`, but the local `status` remains technically successful.

### What ask(...) does

Check functions are called through `ask(...)`. The `ask(...)` macro accepts an expression that returns `Return`, checks the technical part of the result, reads the `YES` or `NO` answer, clears binary flags from the local `status`, and returns a regular C `true` or `false` result.

`ask(...)` expects a local `Return status` in the current scope. It uses that variable to carry the technical part of the check-function return.

The behavior of `ask(...)` can be read this way:

* if the check function returned a normal `YES`, `ask(...)` returns `true`, and local `status` remains technically successful;
* if the check function returned a normal `NO`, `ask(...)` returns `false`, and local `status` also remains technically successful;
* if the check function returned a technical error, `ask(...)` returns `false` and adds that error to local `status`;
* after `ask(...)`, the binary answer is considered handled and must not move further into regular `run()`, `call()`, `provide()`, or `deliver()`.

`ask(...)` is intended for a real check-function return, not for a manually assembled mask in caller code. A correct check function sets `YES` or `NO` itself and exits through `provide(status)` or `deliver(status)`.

This example uses `run(...)` for the first time. In short: `run(print_file(path))` calls `print_file(path)`, merges its `Return` into the local `status`, and normalizes the result. If `print_file()` returns a technical error, the current `status` becomes problematic too. See the detailed explanation below in [“Call chains: run() and call()”](#call-chains-run-and-call).

### Allowed call forms

#### Example 1: direct check-function call

```c
Return print_file_if_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(ask(path_is_readable(path)))
	{
		/* path_is_readable() returned a technical success and YES.
		   ask() consumed YES and returned true.
		   The local status no longer carries the binary answer */

		/* print_file() is a regular Return function.
		   run() may merge its technical result into local status */
		run(print_file(path));

	} else {
		/* ask() returned false in one of two cases.
		   Case 1: path_is_readable() returned SUCCESS | NO.
		   The file is not readable, but local status is still technically successful.
		   Case 2: path_is_readable() returned FAILURE.
		   The local status now contains the technical failure */
	}

	provide(status);
}
```

#### Example 2: separate variable for the check result

`ask(...)` can also be used with an already stored `Return` when that is clearer for reading or debugging. In this form, the check-function result is stored separately, and local `status` receives the technical part only when `ask(readable)` is called.

```c
Return print_file_if_readable_later(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	Return readable = path_is_readable(path);

	/* readable contains the full Return from the check function.
	   ask(readable) consumes the YES or NO answer.
	   Technical FAILURE, if present, is copied into local status */
	if(ask(readable))
	{
		/* The answer was YES and the technical layer was successful */
		run(print_file(path));
	}

	provide(status);
}
```

If the result is stored in a separate variable, consume it near the call. C does not let the library automatically know that a separate local variable was forgotten and will never be used.

#### Example 3: reading an answer already stored in status

Sometimes it is convenient to write the full check-function return directly into local `status` and then read it with `ask(status)`. This is allowed, but the difference matters: `status = path_is_readable(path)` replaces everything that was previously in `status`. Use this style only when that is the intended behavior.

```c
Return print_file_if_readable_resetting_status(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* This assignment replaces the previous local status.
	   After the call, status temporarily contains both the technical layer
	   and the binary YES or NO answer from path_is_readable() */
	status = path_is_readable(path);

	if(ask(status))
	{
		/* ask(status) consumed YES and left only the technical layer.
		   The condition is true only for a technically successful YES */
		run(print_file(path));

	} else {
		/* For SUCCESS | NO, ask(status) returns false and leaves status successful.
		   For FAILURE, ask(status) returns false and leaves status critical */
	}

	provide(status);
}
```

### What counts as an error

`run(...)` and `call(...)` are not meant for check functions that return `YES` or `NO`. If code accidentally writes `run(path_is_readable(path))`, the library reports an error and changes local `status` to `FAILURE`. The same happens when a function receives a binary answer in local `status` and tries to exit through `provide(status)` or `deliver(status)` before handling that answer with `ask(...)`.

The correct algorithm looks like this:

```c
if(ask(path_is_readable(path)))
{
	run(print_file(path));
}
```

The incorrect algorithm looks like this:

```c
run(path_is_readable(path));
```

In the second case, the check function returned an answer to “yes or no”, not a regular work status. `run()` does not accept such answers because that would let `YES` or `NO` leak accidentally through regular call chains. The library reports an error and changes local `status` to `FAILURE`.

If a check-function result should intentionally be discarded completely, use an explicit cast to `void`.

```c
(void)path_is_readable(path);
```

This code deliberately ignores the whole `Return`: both the technical part and the `YES` or `NO` answer. The library does not try to recover a result that has already been discarded.

## Sequential Checks Through status

The same algorithm can be written in several ways. Suppose a function must copy two strings with `strdup()`. `strdup()` allocates memory itself and returns `NULL` on failure. If the first string has already been copied and the second copy fails, the first copy must be released before return.

| Approach | Result |
|---|---|
| nested `if` | works, but code quickly moves to the right and becomes harder to read |
| `goto cleanup` | solves shared cleanup, but adds a forbidden jump style |
| sequential `status` | keeps code flat, readable, and clear about final cleanup |

The first version works, but it quickly turns into a ladder of nested conditions. The more steps the function has, the farther the code drifts to the right.

```c
Return copy_pair_nested(char **first_out, char **second_out, const char *first, const char *second)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *first_copy = NULL;
	char *second_copy = NULL;

	if((first_out == NULL) || (second_out == NULL) || (first == NULL) || (second == NULL))
	{
		status = FAILURE;

	} else {
		first_copy = strdup(first);

		if(first_copy == NULL)
		{
			status = FAILURE;

		} else {
			second_copy = strdup(second);

			if(second_copy == NULL)
			{
				free(first_copy);
				status = FAILURE;

			} else {
				*first_out = first_copy;
				*second_out = second_copy;
			}
		}
	}

	provide(status);
}
```

The second version is common in C code: every failure jumps to one shared cleanup block with `goto`. This solves memory cleanup, but adds a separate jump mechanism. In modern code across projects, `goto` is considered bad practice and should be fully excluded. This is why `librational` uses an alternative mechanism: sequential checks through `status`.

```c
Return copy_pair_goto(char **first_out, char **second_out, const char *first, const char *second)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *first_copy = NULL;
	char *second_copy = NULL;

	if((first_out == NULL) || (second_out == NULL) || (first == NULL) || (second == NULL))
	{
		status = FAILURE;
		goto cleanup;
	}

	first_copy = strdup(first);
	if(first_copy == NULL)
	{
		status = FAILURE;
		goto cleanup;
	}

	second_copy = strdup(second);
	if(second_copy == NULL)
	{
		status = FAILURE;
		goto cleanup;
	}

	*first_out = first_copy;
	*second_out = second_copy;
	first_copy = NULL;
	second_copy = NULL;

cleanup:
	free(second_copy);
	free(first_copy);
	provide(status);
}
```

The third version uses the same principle as `path_is_readable()`: each next step runs only while the current `status` still contains a successful technical result. Temporary pointers are always cleaned up at the end. If everything succeeds, ownership is transferred outward and local pointers are reset so `free()` removes nothing.

```c
Return copy_pair_status(char **first_out, char **second_out, const char *first, const char *second)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *first_copy = NULL;
	char *second_copy = NULL;

	if((first_out == NULL) || (second_out == NULL) || (first == NULL) || (second == NULL))
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		first_copy = strdup(first);

		if(first_copy == NULL)
		{
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		second_copy = strdup(second);

		if(second_copy == NULL)
		{
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		*first_out = first_copy;
		*second_out = second_copy;
		first_copy = NULL;
		second_copy = NULL;
	}

	free(second_copy);
	free(first_copy);
	provide(status);
}
```

This keeps code flat, readable from top to bottom, and free of a separate emergency-exit path. All control flow is built around one local `status`: if a step fails, the next work steps simply do not run, and final cleanup remains in one obvious place.

## Call chains: run() and call()

`run(func)` is for regular work steps that make sense only while the function is still proceeding normally. You can read it as: “if there has not already been an error, warning, stop, or another status that blocks the work chain, run the next step”.

After the call, `run()` adds the result of `func` into the local `status` and normalizes it. This means the next `run()` line sees the updated state and will be skipped when needed.

`call(func)` is for actions that must happen anyway. It does not check the local `status` before running `func`, and it does not decide whether the work chain is still allowed to continue. But after `func` runs, its return value is still added to the local `status` and affects the final function result. That makes `call()` useful for cleanup: release memory, close a file, remove a temporary object, or print a final message.

In short: `run()` is for work that may be skipped after a failure. `call()` is for required cleanup and final actions.

The only mandatory condition is that `func` must return `Return`. These macros take the returned status, add it to the local `status`, and normalize the result. Unfortunately, they cannot be used directly with functions that return `void`, `bool`, `int`, or any other type. Such functions need a small wrapper that returns `Return`.

Check functions that return `YES` or `NO` are not called through `run()` or `call()`. They use `ask(...)`, because the binary answer must be handled immediately next to the call. If such an answer accidentally reaches `run()` or `call()`, the library reports an error and sets `FAILURE`.

```c
Return process_file(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Work steps.
	   open_input() runs first.
	   If it returns FAILURE, WARNING, INFO, or HALTED, local status receives that flag.
	   These flags are SKIP statuses, so later run() calls below will not execute */
	run(open_input(path));
	run(read_input());
	run(write_output());

	/* Cleanup steps.
	   call() does not check local status before running the function.
	   Even if status is already FAILURE or another non-SUCCESS value,
	   both close_input() and close_output() will still be executed */
	call(close_input());
	call(close_output());

	provide(status);
}
```

If `open_input()` returns `FAILURE`, the later work steps `read_input()` and `write_output()` will not run. But `close_input()` and `close_output()` still run because they are called through `call()`.

## Global status

A regular `Return status` describes the state of one specific function. When that function returns, its local `status` is finished too.

`global_return_status` is for program-level events. These events did not necessarily happen inside the current function, but every later return should still see them. A typical example is the user pressing Ctrl+C. The signal handler does not know which function is running right now, but it can set the shared `HALTED` flag.

After that, any function that exits through `provide()` or `deliver()` receives that global context in its return value. `run()` and `call()` also see it after status normalization.

Important: only flags from `GLOBAL` are copied from `global_return_status` into a regular return: `INFO`, `WARNING`, and `HALTED`. Local binary answers `YES` and `NO` do not propagate through global status.

The shortest form looks like this:

```c
atomic_store(&global_return_status,HALTED);
```

After that, the nearest return through `provide()` or `deliver()` receives `HALTED` as global context.

```c
#include <signal.h>
#include <stdatomic.h>

#include "rational.h"

static size_t processed_items = 0;

void handle_sigint(int signal_number)
{
	(void)signal_number;

	/* Ctrl+C requests controlled shutdown for future returns */
	atomic_store(&global_return_status,HALTED);
}

Return install_sigint_handler(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct sigaction action = {
		.sa_handler = handle_sigint
	};

	/* SIGINT is the signal usually sent by Ctrl+C */
	if(sigemptyset(&action.sa_mask) == -1)
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		if(sigaction(SIGINT,&action,NULL) == -1)
		{
			status = FAILURE;
		}
	}

	provide(status);
}

Return process_one_item(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Do one small unit of work.
	   The counter makes the example function perform a visible state change */
	processed_items++;

	/* If Ctrl+C was pressed earlier, provide() will merge HALTED from global_return_status */

	provide(status);
}

Return process_items(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Register Ctrl+C handler before the work loop starts.
	   If handler setup fails, status becomes FAILURE and the loop will not run */
	run(install_sigint_handler());

	while(((SKIP & status) == 0) && items_left())
	{
		/* Before Ctrl+C: process_one_item() returns SUCCESS and the loop continues.
		   After Ctrl+C: the OS calls handle_sigint(), which stores HALTED in global_return_status.
		   The next run() calls process_one_item(), normalizes status, merges HALTED into local status,
		   and the next loop check stops early because HALTED is a SKIP status */
		run(process_one_item());
	}

	/* Cleanup still runs even after HALTED was added to local status */
	call(close_items());

	provide(status);
}
```

## Technical Details

This section describes internal librational mechanisms. In regular user code, they are hidden behind `provide()`, `deliver()`, `run()`, `call()`, and `ask(...)`, so there is no need to use them directly.

These technical details stay in the README for cases where the library itself needs to be improved, its behavior needs to be verified, or a specific return result needs deeper investigation.

### Normalization

Before a function exits, its status goes through internal normalization. Normalization removes contradictory combinations:

* if `CRITICAL` is set, `SUCCESS` is removed;
* if `NO` is set, `YES` is removed;
* `global_return_status` is also normalized and stored back;
* after `GLOBAL` is merged, the status is normalized once again.

This means code may accidentally build `SUCCESS | FAILURE`, but that combination should not leave a function as “both success and failure”. A problematic technical flag is stronger than `SUCCESS`.

The same rule applies to the binary layer: if `YES | NO` appears, `NO` remains. If at least one check answered “no”, the final binary answer can no longer be “yes”.

### Internal YES/NO Algorithm

User code usually sees only `YES`, `NO`, and `ask(...)`. Internally, the library adds one service step: it marks a binary answer as waiting to be handled. This uses the internal `AWAITING` flag.

This flag is not needed in regular application code. The library uses it to distinguish two different states:

* a check function has just returned `YES` or `NO`, and that answer still needs to be consumed with `ask(...)`;
* caller code has already consumed the answer, and only the regular technical `status` should continue through the call chain.

The flow looks like this:

```text
check function -> SUCCESS | YES or SUCCESS | NO
provide()/deliver() -> marks the answer as waiting for ask(...)
ask(...) -> reads YES/NO, returns true/false, clears binary flags
regular code -> continues with technical status only
```

For example, `path_is_readable()` from [“Check function with YES and NO”](#check-function-with-yes-and-no) sets `YES` or `NO` itself. When it exits through `provide(status)`, the library understands that this is a binary answer from a check function and must not be accidentally mixed into a regular call chain.

Caller code should then do this:

```c
if(ask(path_is_readable(path)))
{
	run(print_file(path));
}
```

In this form, `ask(...)` performs all answer handling:

* checks that the function really returned a waiting binary answer;
* reads which answer was returned: `YES` or `NO`;
* returns a regular C `true` or `false` result;
* copies technical errors into local `status`;
* clears binary flags so they are not inherited further.

If `path_is_readable(path)` returned `SUCCESS | YES`, the condition is true. If it returned `SUCCESS | NO`, the condition is false, but local `status` stays successful. If it returned `FAILURE`, the condition is also false, and local `status` becomes critical.

The protection mechanism exists for incorrectly built algorithms. For example:

```c
run(path_is_readable(path));
```

This code tries to pass a binary answer into `run()`, but `run()` is for regular work steps. In this case, the library reports an error and changes local `status` to `FAILURE`. The same rule applies to `call()`.

Another protected case is leaving a function with an unhandled binary answer:

```c
status = path_is_readable(path);

provide(status);
```

Here the function received `YES` or `NO`, but it did not call `ask(status)`. Because of that, `provide(status)` does not let this value leave as a regular return. It reports an error and returns `FAILURE`.

The correct form for that style is shown in [the third result-reading example](#example-3-reading-an-answer-already-stored-in-status):

```c
status = path_is_readable(path);

if(ask(status))
{
	run(print_file(path));
}
```

An explicit cast to `void` is a separate case:

```c
(void)path_is_readable(path);
```

This code intentionally discards the whole function return: the technical part and `YES` or `NO`. After such a call, the library no longer sees the result and does not try to recover it. This is expected behavior when the programmer really wants to ignore the answer completely.

## Practical rules

1. A regular function starts with `Return status = SUCCESS`.
2. Function return goes through `provide(status)` or `deliver(status)`.
3. `FAILURE` is used only for technical errors inside a function.
4. `YES` and `NO` are used only for a local logical answer from a check function.
5. `YES` and `NO` are returned outward only when the current function itself is a check function and must answer “yes” or “no”. These are local flags that do not need to be inherited automatically through a chain of function calls.
6. Caller code reads check functions through `ask(...)`.
7. `run()` is used for work steps that may be skipped after an error or stop.
8. `call()` is used for cleanup and mandatory final actions.
9. `run()` and `call()` are not used for check functions that return `YES` or `NO`.
10. If a status can contain several bits, exact comparison with one flag value is not used. Compound returns are checked with bit masks.
