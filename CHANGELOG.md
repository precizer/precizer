# Changelog

All notable changes will be documented in this file

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
