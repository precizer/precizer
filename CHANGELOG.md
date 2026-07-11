# Changelog

All notable changes will be documented in this file

# Release 0.17.0 2026-07-10

- Long-running SHA512 calculations now periodically save resumable progress in the database. After an unexpected interruption, the next run with `--update` can continue from the latest saved point instead of hashing the file from the beginning, provided its metadata has not changed

# Release 0.16.0 2026-07-01

- Experimental Windows support. The build is not code signed yet. The ZIP archive includes the executable and the required DLL dependency. A portable EXE is also available. It does not require any dependencies, but Defender flags it immediately.

# Release 0.15.3 2026-06-26

- Improved performance when updating the database with `--update/-u` by traversing files that have already been saved to the database.

# Release 0.15.2 2026-06-08

- Improved internal resilience, build and packaging workflows, documentation, and test reliability

# Release 0.15.1 2026-06-07

## Fixed

- Fixed a terminal output issue where rare file names containing control characters could make later scan messages disappear from the screen. Such bytes are now printed as readable `\xNN` escapes, while normal Unicode file names remain readable

# Release 0.15.0 2026-06-06

## Fixed

- File system scans are more resilient: precizer now continues working when an access check fails for one file or directory, treating that path as inaccessible instead of stopping the whole scan

# Release 0.14.0 2026-06-04

## Changed

- `--lock-checksum` now protects locked records from being silently removed when files disappear or become unreadable
- Locked checksum protection now takes priority over `--ignore`, `--include`, `--db-drop-ignored`, and `--db-drop-inaccessible`
- `--rehash-locked` now verifies locked files even when they also match `--ignore`
- Inaccessible files are handled more safely and are kept in the database unless `--db-drop-inaccessible` is explicitly used
- Path and database handling is more reliable, with safer SQL parameter binding and transactional root path saving
- PCRE2 filters can now use JIT compilation for faster matching on large file trees
- Most file and path operations in the main application tests now use internal C helpers instead of external shell commands
- Build and CI support is broader, including dynamic tests, Docker matrix checks, Clang selection, and improved cleanup

## Documentation

- Documentation now explains locked checksum behavior, deep locked-file verification, and inaccessible-file cleanup more clearly

# Release 0.13.0 2026-05-10

## Removed
- CMocka is no longer required to build precizer from source or to run the test suite

# Release 0.12.0 2026-03-19

## Added
`--compare` now supports the `--ignore` and `--include` path filters, so database comparisons can be limited to the paths that matter. Both options can be repeated, and `--include` takes precedence when a path matches both filters

## Documentation
Clarified in the README and CLI help that `--compare` reports, summaries, and equality messages are evaluated against the filtered comparison scope

# Release 0.11.0 2026-03-16

## Changed
- Refined `--silent` behavior in `--compare` mode: compare results remain visible
- Renamed `--compare-filter` values `first-source-only` and `second-source-only` to `first-source` and `second-source` to make combined filter usage clearer. The old values are no longer accepted

# Release 0.10.0 2026-03-14

## Changed
- All releases are now signed and will be distributed with digital signatures

# Release 0.9.0 2026-03-12

## Changed
- Improved `--compare` output so differences between databases are easier to understand, including clearer diagnostics when `--compare-filter` is used
- Messages related to files matched by `--lock-checksum` are now repeated at the end of the run, making important scan results easier to notice in long output
- Updated the bundled SQLite library to a newer upstream version

## Fixed
- Improved compatibility when building precizer on macOS

# v0.8.0 2026-03-05

## Changed
- precizer is now distributed under the GPLv3 license

# v0.7.0 2026-02-28

This version mainly improves test reliability (including complex `--ignore`/`--include`, interruption, and file metadata change scenarios) and includes minor output and documentation polish without noticeable user-facing CLI behavior changes

# v0.6.0 2026-02-10

## Added
- Added reporting for filesystem traversal time and the average per second throughput for reading files and calculating checksums.
- Added `--dry-run=with-checksums` mode for `--dry-run`. In this mode, files are read and checksums are calculated without writing to the database.
- Added `--compare-filter` for `--compare` output scoping (`checksum-mismatch`, `first-source-only`, `second-source-only`) with support for repeated combined filters.

## Changed
- Disk usage calculation now uses allocated block count (`st_blocks`) instead of logical file length (`st_size`). This reflects actual on‑disk space usage rather than apparent size.
- Database format upgraded to version 4 to support the new accounting model; the migration is fully automatic and transparent for users.

## Documentation
- Added a `TROUBLESHOOTING` section to `README.md` with guidance for diagnosing slow filesystem walks, checksum computation, and SQLite `.db` write performance.

# v0.5.0 2026-01-29 Pull Request #45

## Added
- New CLI option `--quiet-ignored` to suppress log lines for files filtered out via `--ignore`/`--include`.
- New CLI option to drop database records for inaccessible files during `--update`:
  - Canonical: `--db-drop-inaccessible`
  - Backward-compatible alias: `--drop-inaccessible`
- Introduced/expanded CMocka-based testing infrastructure.

## Changed
- When `--progress` is enabled, critical warnings/errors collected during filesystem traversal are saved and printed as a single block near the end of the run (to avoid losing important messages in noisy output).
- `--dry-run` can now work with read-only databases by switching away from `ATTACH` and using temporary `TEMP` tables instead.
- Directory traversal approach was reworked.
- Database-related option naming was standardized to the `--db-*` style:
  - Canonical: `--db-drop-ignored`
  - Backward-compatible alias: `--db-clean-ignored`

## Fixed
- File read errors during SHA512 calculation are now treated as warnings rather than hard failures (improves robustness when some files can’t be read).
- Miscellaneous output / logging fixes.

## Tests
- Improved tests around `--rehash-locked`, `--lock-checksum`, and `--watch-timestamps`, plus internal test harness adjustments.

## Documentation
- Added a stable permalink to README for downloading the latest release
