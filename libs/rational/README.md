[<img src="../../.html/img/i18n-icon.svg"> Ссылка на русскоязычную страницу README](README.ru.md)

# librational — shared return statuses for C code

`librational` is a small internal library with shared flags, return macros, logging, and helper functions. Its most visible part is the `Return` type, which lets a function return several independent signals in one value instead of one flat numeric code.

This is useful when code needs to know separately:

* whether a technical error happened inside the function;
* whether the function completed normally;
* what logical answer the function produced, if it checked something;
* whether a global process context should affect later returns.

## Core idea

A plain `int` return code often mixes different meanings. For example, `access()` returns `0` when a file is accessible and `-1` when it is not accessible or when an error happened. That is enough for a small API, but in a larger application it is useful to distinguish:

* the function itself worked correctly, but the check produced a negative answer;
* the function hit an internal technical problem;
* the program is already in a global stopped or warning state.

`Return` handles this with bit flags. One returned value can look like this:

```c
SUCCESS | GOOD
```

It means: the function completed without an internal error, and the logical check result is positive.

Another example:

```c
SUCCESS | NOTGOOD
```

It means: the function completed without an internal error, but the logical check result is negative. For example, a file is not accessible, a record was not found, or a condition is not satisfied.

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

### Binary layer

The binary layer is for yes/no answers without mixing them with technical success or technical failure.

| Flag | Meaning |
|---|---|
| `GOOD` | local answer from a check function: yes, the condition is true |
| `NOTGOOD` | local answer from a check function: no, the condition is false |
| `BOOLEAN` | binary flag mask: `GOOD | NOTGOOD` |

Example: a function checks whether a path is readable. If the check runs normally and the path is readable, it can return `SUCCESS | GOOD`. If the check runs normally but the path is not readable, it can return `SUCCESS | NOTGOOD`.

A simple rule: `GOOD` and `NOTGOOD` fit functions that ask a question by their meaning. For example: `is_*`, `has_*`, `can_*`, `check_*`, `validate_*`. Caller code usually reads that answer right after the call. If the current function has already decided what to do next, the binary answer has been handled. Return `GOOD` or `NOTGOOD` upward only when the current function itself also promises a yes/no answer.

### Global layer

The global layer is for process state that can be set outside the current function. For example, a signal handler can set `global_return_status` to `HALTED` so later returns know that the program should stop.

| Flag | Meaning |
|---|---|
| `INFO` | informational result. For example, the application printed `--help`, `--version`, or another help screen and exited normally |
| `WARNING` | warning that should be visible outside one function |
| `HALTED` | process is stopped or should stop. For example, the user pressed Ctrl+C and the signal handler requested shutdown |
| `GLOBAL` | mask of flags that may propagate from `global_return_status` into a regular return |

Only `GLOBAL` bits are propagated from `global_return_status` into a function result. Binary answers `GOOD` and `NOTGOOD` do not leak between functions through global status.

## Normalization

Before a value is returned, it goes through `normalize_return_status()`. Normalization removes contradictory combinations:

* if `CRITICAL` is set, `SUCCESS` is removed;
* if `NOTGOOD` is set, `GOOD` is removed;
* `global_return_status` is also normalized and stored back;
* after `GLOBAL` is merged, the status is normalized once again.

This means code may accidentally build `SUCCESS | FAILURE`, but that combination should not leave a function as “both success and failure”. A problematic technical flag is stronger than `SUCCESS`.

The same rule applies to the binary layer: if `GOOD | NOTGOOD` appears, `NOTGOOD` remains. If something returned a bad logical result, the final binary answer can no longer be considered good.

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

## Check function with GOOD and NOTGOOD

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
			status |= GOOD;
		} else {
			/* The check ran normally and the answer is negative */
			status |= NOTGOOD;
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

Check the technical layer first. Only then read the binary answer. In the example below, `path_is_readable()` returns `GOOD` or `NOTGOOD` because its meaning is to answer the question “is this file readable?”. `print_file_if_readable()` no longer answers that question. It only decides whether to print the file, so it does not return `GOOD` or `NOTGOOD` further upward.

This example uses `run(...)` for the first time. In short: `run(print_file(path))` calls `print_file(path)`, merges its `Return` into the local `status`, and normalizes the result. If `print_file()` returns a technical error, the current `status` becomes problematic too. See the detailed explanation below in [“Call chains: run() and call()”](#call-chains-run-and-call).

```c
Return print_file_if_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	Return readable = path_is_readable(path);

	if(CRITICAL & readable)
	{
		/* The check itself failed.
		   This function cannot continue either */
		provide(FAILURE);
	}

	if(GOOD & readable)
	{
		/* GOOD is used only as a local decision.
		   The file is readable, so this function may print it */
		/* run() merges print_file() return status into local status */
		run(print_file(path));

	} else {
		/* In this example NOTGOOD is not interesting as a returned answer.
		   It only means there is nothing to print */
	}

	provide(status);
}
```

For a short technical-success check, use `TRIUMPH`. The logic is the same: a technical error becomes `FAILURE` for the current function, and `GOOD` is used only for the local decision.

```c
Return result = path_is_readable(path);

if((TRIUMPH & result) == 0)
{
	/* There is no successful or graceful technical outcome */
	return(FAILURE);
}

if(GOOD & result)
{
	/* The condition is true, so the caller may do the useful work */
	return(print_file(path));
}

/* NOTGOOD is handled here as "nothing to do", not as a returned answer */
return(SUCCESS);
```

The main rule: do not read `NOTGOOD` as a technical failure. It is a negative logical answer. Technical failure is checked through `CRITICAL`, `FAILURE`, `WARNING`, or through the absence of the expected `TRIUMPH`. Do not pass `GOOD` and `NOTGOOD` through a call chain automatically. If a function received a binary answer and already chose an action from it, that binary answer has been handled for this function.

## Sequential Checks Through status

The same algorithm can be written in several ways. Suppose a function must copy two strings with `strdup()`. `strdup()` allocates memory itself and returns `NULL` on failure. If the first string has already been copied and the second copy fails, the first copy must be released before return.

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

Important: only flags from `GLOBAL` are copied from `global_return_status` into a regular return: `INFO`, `WARNING`, and `HALTED`. Local binary answers `GOOD` and `NOTGOOD` do not propagate through global status.

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

## Practical rules

1. A regular function starts with `Return status = SUCCESS`.
2. Function return goes through `provide(status)` or `deliver(status)`.
3. `FAILURE` is used only for technical errors inside a function.
4. `GOOD` and `NOTGOOD` are used only for a local logical answer from a check function.
5. `GOOD` and `NOTGOOD` are returned outward only when the current function itself is a check function and must answer “yes” or “no”. These are local flags that do not need to be inherited automatically through a chain of function calls.
6. Caller code checks the technical layer first, then the binary answer.
7. `run()` is used for work steps that may be skipped after an error or stop.
8. `call()` is used for cleanup and mandatory final actions.
9. If a status can contain several bits, exact comparison with one flag value is not used. Compound returns are checked with bit masks.
