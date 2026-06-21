# Repository Guidelines

## Comments and Documentation
- Use US English for comments and documentation
- If a comment or documentation block contains multiple sentences, separate them with periods. However, do not place a trailing period at the end of the last sentence in a block, at the end of a single-line comment, or at the end of a single-line documentation entry
- Preserve existing comments in their current locations. They may be updated for accuracy or clarity, but must never be removed
- Write function documentation in Doxygen style
- Write function documentation in the `.c` file that contains the function body, not in the header file
- Human-understandable documentation is mandatory for function documentation and for README documentation, including `README.md`, `README.ru.md`, and bundled-library READMEs under `libs/`
- For README documentation pairs, `README.ru.md` is the original source of truth and `README.md` is its US English translation
- Documentation must be easy to understand for a person who does not know the internal project context, implementation history, or author intent
- Clarity, practical usefulness, and ease of understanding take priority over formal completeness alone
- Comments and documentation must describe the current behavior of the code, not the history of how it changed
- Do not mention older implementations, removed helpers, or previous behavior unless documenting compatibility, migration, or deprecation
- Include technical details only when they clarify correctness, safety, or non-obvious constraints

# Indentation
Use tabs for indentation. Spaces may be used for alignment when needed, for example when a line with all function arguments would otherwise be too long, or inside function documentation blocks and multiline comments.

# Change Policy
- Provide a numbered implementation plan for approval before making any code changes, and do not modify code until the plan is explicitly approved
- If a change only affects function documentation or comments, including adding, updating, or refreshing them, no plan approval is required and the work should begin immediately
- This exception does not apply to `README.md` or `README.ru.md`
- Perform file renames and deletions using git commands only (`git mv` and `git rm`)
- If a build or test fails after a change, stop making further changes immediately, explain the cause, and propose possible solutions. Do not modify code or tests merely to work around the failure without explicit user approval. After a successful implementation, carefully review the changes and repeat the verification-and-correction cycle until the result is reliable and maintainable
- If a function body contains a `/* ... */` comment stating that the function was reviewed line by line by a human and requires separate explicit approval for changes, treat that statement as a mandatory repository rule
- Any modification of such a function requires separate explicit human approval before the change is made
- When an AI is asked to modify such a function, it must explicitly remind the human that the changed function must be reviewed again line by line by a human before it is considered trusted

## Project Structure & Module Organization
- `src/`: main application source (`precizer.c`, argument parsing, DB/checksum logic)
- `Makefile`: root build entrypoint
- `libs/`: internal libraries (`sqlite3`, `sha512`, `mem`, `rational`, `xdiff`, `testitall`), each with its own `Makefile` and `src/`
- `libs/sqlite3/src/` contains third-party SQLite amalgamation sources (`sqlite3.c` and header). Treat this as vendor code and never modify it
- `tests/`: hybrid test suite (`tests/src/`, golden outputs in `tests/templates/`, filesystem fixtures in `tests/fixtures/`)
- `tools/`: auxiliary developer utilities
- `.builds/`: generated build/test artifacts (do not commit)

## Build, Test, and Development Commands
- For routine Codex verification after changes, run only `SLOWTEST=skip make tests-debug`. It uses debug flags, avoids sanitizer runtime constraints, and skips non-essential long-running tests
- When a full test run is needed, run only `make tests-debug`. It includes the slow scenarios
- Do not run multiple test variants sequentially by default. Use `make tests` only when sanitizer verification is specifically requested or justified by the change
- The main application Makefile and internal library Makefiles usually collect source files with `$(wildcard src/*.c)`, so newly added `.c` files are picked up automatically after creation or `git mv`
- `make` or `make production`: default optimized build; outputs `./precizer`.
- `make portable`: static portable Linux build (UPX-compressed).
- `make dynamic-production`: dynamic build using system `sqlite3`/`pcre2`.
- `make debug`: build the application with debug flags.
- `make sanitize`: build the application with ASan/UBSan.
- `make tests-debug`: build and run the debug test suite, including slow scenarios unless `SLOWTEST=skip` is set.
- `make tests`: build and run the sanitizer-backed test suite via `tests/Makefile`; use it only when sanitizer verification is specifically needed.
- `make coverage`: builds and runs the coverage-instrumented test suite, then generates the coverage report.
- `make purge`: remove build outputs and generated binaries, all artifacts in .builds/

## Coding Style & Naming Conventions
- Language standard is `C2x`; builds use strict warnings and `-Werror`.
- Formatting is enforced with `uncrustify` (`Uncrustify.cfg`)
- After all changes are complete, run `uncrustify` individually for each changed source or header file. Do not use global formatting targets for agent changes
- Follow existing naming: lowercase snake_case for files/functions (for example `db_check_changes.c`, `parse_arguments`).
- Keep changes localized; avoid broad refactors in feature PRs.
- Variable, function, and struct names should be self-explanatory, even if that makes them long

## Testing Guidelines
- When an AI agent needs the application binary without running the test suite, prefer `make debug`, which produces `.builds/debug/precizer` with debug symbols
- Default test command for agent changes: `SLOWTEST=skip make tests-debug`
- Full test command when the change warrants slow scenarios or before opening a pull request: `make tests-debug`
- Choose one test command based on the required coverage. Do not run the default and full commands consecutively
- Add/adjust tests in `tests/src/` (pattern: `testXXXX.c`).
- Update expected outputs in `tests/templates/` when behavior changes.
- Add fixture data under `tests/fixtures/` only when needed for reproducible cases.

## Commit & Pull Request Guidelines
- Use one branch and one PR per logical change.
- Commit messages in repository style: short, descriptive, sentence-case (example: `Optimize header file includes`).
- PR description should include:
  1. problem being solved;
  2. exact scope of changes;
  3. validation commands run (for example, `make tests-debug`);
  4. known limitations/follow-ups.

# Standard Return Statuses and librational
- Never use `goto`; avoid ternary operators when practical
- Regular functions return `Return` and start with a default local status:
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
- `FAILURE` is used only for technical errors inside a function: out of memory, damaged state, or inability to continue. A normal negative logical answer is not `FAILURE`
- Check functions may return `SUCCESS | YES` or `SUCCESS | NO`. `YES` and `NO` are local binary answer flags, not C `bool` values; `NO` is not equal to `0`. Return them outward only from functions that themselves answer a yes/no question
- Caller code consumes check-function results with `ask(check_function(...))`. `ask()` returns a C `bool`, merges technical failures into local `status`, and clears the binary answer so it is not inherited through the call chain
- As a function progresses, its local `status` accumulates return flags from completed operations. Guard non-cleanup logical blocks with a condition such as `if(SUCCESS == status)`, `if(SUCCESS & status)`, or `if(TRIUMPH & status)`. Choose the condition individually for each block based on the flags that may be present in `status` immediately before that block
- Status guards allow execution to reach shared cleanup code after an earlier failure while skipping operations that should no longer run. Perform required cleanup unconditionally or through `call(...)`, then return the accumulated status through `provide(status)` or `deliver(status)`
- `run(func)` is for work steps that may be skipped after a failure or stop. `func` must return `Return` and must not be a yes/no check function. `run()` calls `func` only while local `status` does not contain any flag from the `SKIP` mask, then adds the return from `func` into `status` and normalizes it
- `call(func)` is for cleanup and mandatory final actions. `func` must return `Return` and must not be a yes/no check function. `call()` always calls `func`, even when local `status` is already not `SUCCESS`, but the return from `func` is still added into `status` and affects the final result
- Function return goes through `provide(status)` or `deliver(status)`. `provide()` additionally writes TRACE for a critical return; `deliver()` returns without that TRACE
- `global_return_status` is for program-level events. Only `GLOBAL` flags propagate from it into a regular return: `INFO`, `WARNING`, `HALTED`. `YES` and `NO` do not propagate through global status
- If a status can contain several bits, do not compare the whole value exactly with one flag. Use bit masks for compound returns
