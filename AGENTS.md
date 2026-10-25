# Repository Guidelines

## Comments and Documentation
- Use US English for comments and documentation
- Use an impersonal style in documentation. Avoid addressing the reader directly or using first-person pronouns
- If a comment or documentation block contains multiple sentences, separate them with periods. However, do not place a trailing period at the end of the last sentence in a block, at the end of a single-line comment, or at the end of a single-line documentation entry
- Preserve existing comments in their current locations. They may be updated for accuracy or clarity, but may be removed only when the code they describe is also deleted
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

## Request Interpretation

- A question alone is not an instruction, approval of a solution, or authorization to change code or documentation
- Questions about possible solutions are requests for explanation or continued exploration and comparison of alternatives. Discussed alternatives may conflict, so they must not be combined or implemented automatically
- Changes to code or documentation require an explicit instruction, unambiguous approval of a specific change, or a clarification that directly requests the change
- Without such authorization, only an explanation, analysis, or comparison is provided, and code and documentation remain unchanged

## Approval and Implementation

- Provide a numbered implementation plan for approval before making any code changes, and do not modify code until the plan is explicitly approved
- For an authorized change that only affects function documentation or comments, including adding, updating, or refreshing them, no separate plan approval is required and the work should begin immediately
- This exception does not apply to `README.md` or `README.ru.md`
- Perform file renames and deletions using git commands only (`git mv` and `git rm`)
- If a build or test fails after a change, stop making further changes immediately, explain the cause, and propose possible solutions. Do not modify code or tests merely to work around the failure without explicit user approval. After a successful implementation, carefully review the changes and repeat the verification-and-correction cycle until the result is reliable and maintainable
- Functions marked as reviewed line by line by a human may be modified within the authorized task without separate permission. This rule supersedes requirements for separate approval in function comments
- Any modification invalidates the function's previous human review. Do not claim that the modified function is reviewed or trusted until that review is confirmed
- After modifying any such function, the final response must explicitly notify the developer, list each affected function with its file location, summarize what changed, and state that a new line-by-line human review is required before the function is considered trusted. This notification is mandatory even for minor changes and must not be replaced by a generic change summary or an earlier progress update

## Project Structure & Module Organization
- `src/`: main application source (`precizer.c`, argument parsing, DB/checksum logic)
- `Makefile`: root build entrypoint
- `libs/`: internal and bundled libraries (`sqlite3`, `sha512`, `mem`, `rational`, `xdiff`, `testitall`, `monocypher`), each with its own `Makefile` and `src/`
- `libs/sqlite3/src/` and `libs/monocypher/src/` contain third-party vendored sources. Treat this as vendor code and never modify it directly
- `tests/`: hybrid test suite (`tests/src/`, golden outputs in `tests/templates/`, filesystem fixtures in `tests/fixtures/`)
- `tools/`: auxiliary developer utilities
- `.builds/`: generated build/test artifacts (do not commit)

## Build, Test, and Development Commands
- For Codex verification after changes, run `make tests-debug`. It uses debug flags and runs the complete test suite
- Do not run multiple test variants sequentially by default. Use `make tests` only when sanitizer verification is specifically requested or justified by the change
- The main application Makefile and internal library Makefiles usually collect source files with `$(wildcard src/*.c)`, so newly added `.c` files are picked up automatically after creation or `git mv`
- `make` or `make production`: default optimized build; creates a ZIP package in the project root
- `make portable`: static portable Linux build; creates a ZIP package with an UPX-compressed executable
- `make dynamic-production`: dynamic build using system `sqlite3`/`pcre2`; creates a ZIP package in the project root
- `make distribution`: build a binary for distribution packages at `.builds/distribution/precizer`
- `make macos-zip`: build and package the macOS release using the distribution build rules
- `make debug`: build the application with debug flags.
- `make sanitize`: build the application with ASan/UBSan.
- `make tests-debug`: build and run the complete debug test suite
- `make tests`: build and run the sanitizer-backed test suite via `tests/Makefile`; use it only when sanitizer verification is specifically needed.
- `make coverage`: builds and runs the coverage-instrumented test suite, then generates the coverage report.
- `make clean`: remove build outputs, ZIP packages, attestation bundles, and package staging directories
- `make purge`: remove build artifacts in `.builds/` while preserving release packages in the project root

## Coding Style & Naming Conventions
- Language mode is `GNU C2x` (`-std=gnu2x`); builds use strict warnings and `-Werror`
- Formatting is enforced with `uncrustify` (`Uncrustify.cfg`)
- Follow existing naming: lowercase snake_case for files/functions (for example `db_check_changes.c`, `parse_arguments`).
- Keep changes localized; avoid broad refactors in feature PRs.
- Variable, function, and struct names should be self-explanatory, even if that makes them long
- Compare `bool` variables explicitly with `true` or `false` in conditions, including compound conditions: `if(value == true)`, `if(value == false)`, and `if(value == true && arg != NULL)`. Do not use `if(value)` or `if(!value)`

## Testing Guidelines
- When an AI agent needs the application binary without running the test suite, prefer `make debug`, which produces `.builds/debug/precizer` with debug symbols
- Test command for agent changes and before opening a pull request: `make tests-debug`. Run the complete suite
- Add/adjust tests in `tests/src/` (pattern: `testXXXX.c`).
- When creating new tests, look first for suitable shared helpers in `tests/src/helpers_db_utils.c` and `tests/src/helpers_file_utils.c`. Add local `static` helpers only when the shared helpers do not already provide the needed behavior
- Update expected outputs in `tests/templates/` when behavior changes.
- Output-oriented tests must use complete golden templates in `tests/templates/` for each checked nonempty stream. Matching selected lines or fragments is insufficient. Comparisons must cover the stream from beginning to end, including its expected line endings, and reject unexpected leading, trailing, or intermediate output
- Each distinct expected output must have a self-contained golden template. Do not assemble golden templates from other templates or output fragments at runtime. Cases with identical expected output may share one complete template
- Golden regular expressions may accommodate variable values such as timestamps, paths, and sizes, but must preserve the expected output structure. Use whole-stream boundaries such as `\A` and `\z`, or `\Z` with explicit handling of the final newline. Line anchors `^` and `$` alone are insufficient. Do not use unrestricted wildcards to skip output: `match_pattern()` enables `PCRE2_DOTALL`, so `.*` can hide extra lines. Constrain variable fields to their intended content, using expressions such as `[^\r\n]+` for single-line values
- Required empty stdout and stderr must be checked explicitly, using `STDOUT_MUST_BE_EMPTY` and `STDERR_MUST_BE_EMPTY` with `run_and_match_templates()`. A NULL template skips that stream and must not be treated as an empty-output check or complete output coverage
- Do not remove output merely to make it match a golden template. An explicitly agreed source-specific difference, such as the configuration-path diagnostic, may be checked separately before comparing the remaining output against a shared full template; every excluded line must have its complete expected content and occurrence count verified
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
