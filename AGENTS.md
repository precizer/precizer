# Repository Guidelines

## Comments and Documentation
- Use US English for comments and documentation
- If a comment or documentation block contains multiple sentences, separate them with periods. However, do not place a trailing period at the end of the last sentence in a block, at the end of a single-line comment, or at the end of a single-line documentation entry
- Write function documentation in Doxygen style
- Write function documentation in the `.c` file that contains the function body, not in the header file
- The main human-readable project documentation is available in `README.md` in US English and in `README.ru.md` in Russian
- `README.ru.md` is the original source of truth for the human-readable project documentation, and `README.md` is its US English translation. This reflects the historical fact that Russian was the original working language of the project documentation
- Bundled libraries under `libs/` may also have their own `README.md` in US English and `README.ru.md` in Russian
- For such bundled-library documentation, `README.ru.md` is also the original source of truth and `README.md` is its US English translation

## Mandatory Documentation Requirement
- Human-understandable and user-friendly documentation is a mandatory requirement
- This requirement applies to function documentation and to all human-readable README documentation, including `README.md`, `README.ru.md`, and bundled-library READMEs under `libs/`
- Documentation must be easy to understand for a person who does not know the internal project context, implementation history, or author intent
- Clarity, practical usefulness, and ease of understanding take priority over formal completeness alone

# Indentation
Tabs only

# Change Policy
- Provide a numbered implementation plan for approval before making any code changes
- Do not modify code until the plan is explicitly approved
- If a change only affects function documentation or comments, including adding, updating, or refreshing them, no plan approval is required and the work should begin immediately
- This exception does not apply to `README.md` or `README.ru.md`
- Perform file renames and deletions using git commands only (`git mv` and `git rm`)
- If a function body contains a `/* ... */` comment stating that the function was reviewed line by line by a human and requires separate explicit approval for changes, treat that statement as a mandatory repository rule
- Any modification of such a function requires separate explicit human approval before the change is made
- When an AI is asked to modify such a function, it must explicitly remind the human that the changed function must be reviewed again line by line by a human before it is considered trusted

## Project Structure & Module Organization
- Main application sources are in `src/` and the root build entrypoint is `Makefile`
- `libs/` contains in-repo internal libraries that can be embedded into each other or into the main app sources in `src/`
- `libs/sqlite3/src/` contains third-party SQLite amalgamation sources (`sqlite3.c` and header). Treat this as vendor code and never modify it

`precizer` is a C2x CLI project with a split layout:
- `src/`: application source (`precizer.c`, argument parsing, DB/checksum logic).
- `libs/`: internal libraries (`sqlite3`, `sha512`, `mem`, `rational`, `xdiff`, `testitall`), each with its own `Makefile` and `src/`.
- `tests/`: hybrid test suite (`tests/src/`, golden outputs in `tests/templates/`, filesystem fixtures in `tests/fixtures/`).
- `tools/`: auxiliary developer utilities.
- `.builds/`: generated build/test artifacts (do not commit).

## Build, Test, and Development Commands
- For local verification, prefer `make tests-debug` because it runs the test suite without sanitizer overhead
- For fast iteration, use `SLOWTEST=skip make tests-debug` to skip non-essential long-running tests
- The main application Makefile and internal library Makefiles usually collect source files with `$(wildcard src/*.c)`, so newly added `.c` files are picked up automatically after creation or `git mv`
- `make` or `make production`: default optimized build; outputs `./precizer`.
- `make portable`: static portable Linux build (UPX-compressed).
- `make dynamic-production`: dynamic build using system `sqlite3`/`pcre2`.
- `make debug` / `make sanitize`: debug or ASan/UBSan build.
- `make tests`: runs sanitizer-backed test suite via `tests/Makefile`, so it is slower and may be sensitive to sanitizer/runtime environment constraints.
- `make coverage`: runs coverage flow (`tests` + report generation).
- `make format && (cd libs && make format) && (cd tests && make format)`: format touched code with `uncrustify`.
- `make purge`: remove build outputs and generated binaries, all artifacts in .builds/

## Coding Style & Naming Conventions
- Language standard is `C2x`; builds use strict warnings and `-Werror`.
- Formatting is enforced with `uncrustify` (`Uncrustify.cfg`): tabs for indentation (`indent_with_tabs = 1`, tab size 4), no backup files.
- Follow existing naming: lowercase snake_case for files/functions (for example `db_check_changes.c`, `parse_arguments`).
- Keep changes localized; avoid broad refactors in feature PRs.
- Variable, function, and struct names should be self-explanatory, even if that makes them long

## Testing Guidelines
- When an AI agent needs a build for running or testing the application, prefer `make debug`, which produces the `.builds/debug/precizer` binary with debug symbols
- Minimum pre-PR check: `make tests`.
- Add/adjust tests in `tests/src/` (pattern: `testXXXX.c`).
- Update expected outputs in `tests/templates/` when behavior changes.
- Add fixture data under `tests/fixtures/` only when needed for reproducible cases.

## Commit & Pull Request Guidelines
- Use one branch and one PR per logical change.
- Commit messages in repository style: short, descriptive, sentence-case (example: `Optimize header file includes`).
- PR description should include:
  1. problem being solved;
  2. exact scope of changes;
  3. validation commands run (for example, `make tests`);
  4. known limitations/follow-ups.

# Стандартные статусы возврата
- Ни в коем случае не использовать goto
- Стараться избегать использования тернарных операторов
- Обычне функции возвращают Return и начинаются с установки флага по умолчанию с соответствующим коментарием:
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
- status = FAILURE ставится только тогда, когда происходит неуправляемая ошибка. Например, не удалось выделить память. Эта ошибка не
  зависит от приложения и его логики.
