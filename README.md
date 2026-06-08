[<img src=".html/img/i18n-icon.svg"> Link to the Russian language README page](README.ru.md)

# Precizer: data integrity verification for file systems of any scale

A Tiny, High-Performance File Integrity and Comparison Tool

“A truly great application will always fit on a floppy disk. Hopefully, someone out there still remembers what those were… But it’s not about the floppies, it’s about quality software!”<sup>©</sup> :-D

<p width="100%" height="100%"><img width="20%" src=".html/img/micrometer_0.svg"></p>

## Continuous integration and automation

### Comprehensive hybrid test suite

* In-process integration tests
* Out-of-process CLI system tests

#### Test coverage:

<a href="https://precizer.github.io/code_coverage_report/"><img src=".html/img/test-coverage-total.svg" height="20" alt="Total completed tests" /><br>
<img src=".html/img/test-coverage-lines.svg" height="20" alt="Lines of code covered by tests" /><br>
<img src=".html/img/test-coverage-functions.svg" height="20" alt="Functions covered by tests" /><br>
<img src=".html/img/test-coverage-branches.svg" height="20" alt="Code branches covered by tests" /></a>

### Automated builds:

[![precizer build & testing](https://github.com/precizer/precizer/actions/workflows/precizer.yml/badge.svg)](https://github.com/precizer/precizer/actions/workflows/precizer.yml)

### Security:

[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/12159/badge)](https://www.bestpractices.dev/projects/12159)
[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/precizer/precizer/badge)](https://scorecard.dev/viewer/?uri=github.com/precizer/precizer)

## TL;DR

### Overview

**precizer** is a lightweight, high-performance CLI tool written in pure C. It’s designed for file integrity verification and comparison, making it especially useful for validating synchronization results. The program walks directory trees and builds a database of files and their checksums for fast, repeatable comparisons.

Built for embedded systems and large-scale clustered environments, **precizer** detects synchronization drift by comparing files and checksums across sources. It can also analyze historical changes by comparing databases captured from the same source at different points in time.

### Basic Example

Consider a scenario where two machines have large mounted volumes at `/mnt1` and `/mnt2`, respectively, containing identical data. The goal is to verify, byte by byte, whether the contents are truly identical or if discrepancies exist.

1. Run **precizer** on the first machine (e.g., hostname `host1`):

```sh
precizer --progress /mnt1
```

This command traverses the directory tree under `/mnt1`, creating a database file `host1.db` in the current directory. The `--progress` flag provides real-time progress updates, displaying the total traversed space and the number of processed files.

2. Run **precizer** on the second machine (e.g., hostname `host2`):

```sh
precizer --progress /mnt2
```

This will generate a database file `host2.db` in the current directory.

3. Copy `host1.db` and `host2.db` to one of the machines and run the following command to compare them:

```sh
precizer --compare host1.db host2.db
```

The output will display:
- Files that exist on `host1` but are missing on `host2`, and vice versa.
- Files present on both hosts but with **different checksums**.

### Relative Paths for Consistent Comparison

**precizer** stores only relative file paths in its database. For example, a file located at:

```
/mnt1/abc/def/aaa.txt
```

will be stored as:

```
abc/def/aaa.txt
```

without the `/mnt1` prefix. Similarly, the corresponding file on `/mnt2`:

```
/mnt2/abc/def/aaa.txt
```

will also be stored as:

```
abc/def/aaa.txt
```

This ensures that even when files reside in different mount points or sources, they can still be compared accurately under the same relative paths and their respective checksums.

## [DOWNLOAD](https://github.com/precizer/precizer/releases/latest/)

Download [https://github.com/precizer/precizer/releases/latest/](https://github.com/precizer/precizer/releases/latest/) executables for:

* Linux x86_64 [precizer_linux_x86_64_portable.zip](https://github.com/precizer/precizer/releases/latest/download/precizer_linux_x86_64_portable.zip)
* Linux arm aarch64 [precizer_linux_aarch64_portable.zip](https://github.com/precizer/precizer/releases/latest/download/precizer_linux_aarch64_portable.zip)
* macOS arm64 [precizer_macos_arm64.zip](https://github.com/precizer/precizer/releases/latest/download/precizer_macos_arm64.zip)

The release packages contain portable executables in a zip archive.

### Download, unzip, and run

A universal approach to automating upgrades to newer versions

```sh
# Automation for downloading and unarchiving new versions

# Download
wget -O precizer.zip -q "https://github.com/precizer/precizer/releases/latest/download/precizer_$(uname -s | tr '[:upper:]' '[:lower:]' | sed 's/darwin/macos/')_$(uname -m | sed 's/amd64/x86_64/')$( [ "$(uname -s)" = "Linux" ] && echo '_portable' ).zip"

# Extract the archive
unzip -jqo precizer.zip '*/precizer' -d ./

# Run
./precizer --version
```

### Technical details of the portable build

* The Linux build is a single executable, statically linked ELF binary not tied to any specific distribution. It can be run immediately on almost any Linux distro and does not require external shared libraries.
* The binary is produced by GitHub CI/CD, then compressed with [UPX (the executable packer)](https://upx.github.io). The self-extracting compressed binary is then placed into a ZIP archive for convenient download. The file can be extracted from the archive and run directly.
* Static linking is not supported on macOS, so running the downloaded application requires the following libraries to be available on the system: sqlite3, pcre2, argp and fts.

## CHANGELOG

A list of changes by version is available in a separate file: [CHANGELOG](CHANGELOG.md)

## TECHNICAL DETAILS

Consider a scenario where a primary storage system has a backup copy. For example, this could be a data center storage and its *Disaster Recovery* copy.

Synchronization from the primary storage to the backup occurs periodically, but due to the *massive data volumes*, synchronization is most likely not performed byte-by-byte but rather by detecting *metadata changes* within the file system. In such cases, *file size* and *modification time* are taken into account, but the actual content is *not verified byte by byte*.

This approach makes sense because the primary data center and the *Disaster Recovery* site usually have *high-speed communication channels*, but a full byte-by-byte synchronization would take an *unreasonably long time*.

Tools like `rsync` allow both types of synchronization — *metadata-based* and *byte-by-byte* — but they have one *major drawback*: *state is not preserved between sessions*.

The following scenario illustrates the issue:

* Given: Server "A" and Server "B" (Primary Data Center and Disaster Recovery)
* Some files have been modified on Server "A".
* The `rsync` algorithm detects them based on changes in size and modification time and synchronizes them to Server "B".
* Multiple connection failures occur during synchronization between the Primary Data Center and the Disaster Recovery site.
* To verify data integrity (i.e., ensuring that files on "A" and "B" are identical byte by byte), `rsync` is often used with byte-by-byte comparison. The process works as follows:
  * `rsync` is launched on Server "A" with the `--checksum` mode, attempting to compute checksums sequentially on both "A" and "B" in a single session.
  * This process takes an extremely long time for large-scale storage systems.
  * Since `rsync` does not save computed checksums between sessions, it introduces several technical challenges:
    * If the connection drops, `rsync` terminates the session, and on the next run, everything must start from scratch! Given the huge data volumes, performing a byte-by-byte verification for full data integrity becomes an impossible task.
  * Storage subsystem failures can also lead to binary inconsistencies. In such cases, file system metadata cannot reliably determine whether file contents on "A" and "B" are truly identical.
  * Over time, errors accumulate, increasing the risk of maintaining an inconsistent Disaster Recovery copy of system "A" on system "B", rendering the entire Disaster Recovery effort useless. Standard utilities do not detect these inconsistencies, and technical personnel may be completely unaware of data integrity problems in the Disaster Recovery storage.
* To overcome these limitations, precizer was developed. The program identifies exactly which files differ between "A" and "B" so that they can be resynchronized with the necessary corrections. The tool operates at maximum speed (pushing hardware performance to its limits) because it is written in pure C and utilizes high-performance algorithms optimized for efficiency. The program is designed to handle both small files and petabyte-scale data volumes, with no upper limits*.
* The name precizer comes from the word precision, implying something that enhances accuracy.
* The program precisely analyzes directory contents, including subdirectories, computing checksums for every encountered file while storing metadata in an SQLite database (a regular binary file).
* precizer is fault-tolerant and can resume execution from the point of interruption. For example, if the program is terminated via Ctrl+C while analyzing a petabyte-scale file, it will NOT restart from the beginning but continue exactly where it left off using previously recorded data in the database. This significantly saves resources, time, and effort for system administrators.
* The program can be interrupted at any time using any method, and this is completely safe for both the scanned data and the database created by precizer.
* If the program is intentionally or accidentally stopped, there is no need to worry about losing progress. All results are fully preserved and can be used in subsequent runs.
* Checksum calculations rely on the cryptographic SHA512 hash algorithm, which is reliable, fast, and provides very strong practical collision resistance. If two large files differ by even one byte, SHA512 will overwhelmingly likely produce different checksums; unlike CRC32 and the now-outdated SHA1, it is designed for robust data integrity verification
* The algorithms in precizer are designed to make it easy to keep the database up to date without having to recalculate everything from scratch. Simply run the program with the `--update` parameter, and new files will be added to the database, while entries for deleted files will be removed. If a file has been modified and its size has changed, its SHA512 checksum will be recalculated and updated in the database.
* During `--update`, entries for missing files are removed, but records for inaccessible files (permission denied) are kept by default. This protection exists because permissions can temporarily change (ownership, ACLs, transient mount issues), and dropping records in that state would silently erase valid database history. Using `--db-drop-inaccessible` with `--update` is intended only when those database records must be dropped.
* When `--progress` is enabled, warnings and errors collected during a session are printed in one block right before exit so important messages (for example, file access issues) are not lost in routine logs.
* The `--quiet-ignored` option suppresses per-file log lines for paths filtered by `--ignore` and `--include`. This helps keep program logs free of extra messages once ignore regular expressions are tuned and stable in use; other warnings and errors remain visible.
* There is an option to consider not only the file size when updating the database but also the file’s creation or modification timestamps. This means that any change in file metadata will trigger an SHA512 checksum recalculation and update in the database. For example, if a file’s ctime changes but its size remains the same, the checksum will NOT be recalculated if only the `--update` parameter is used. To force checksum recalculation for such files `--watch-timestamps` should be added. This option is disabled by default because ctime (like mtime) can change frequently due to commands like `chmod` or `chown`, even when the file’s content remains the same.
* precizer can be used as a security monitoring tool, detecting unauthorized file modifications where contents might have changed while metadata remains untouched.
* Security:
  * The program never modifies, deletes, moves, or copies any files or directories it processes.
  * The program enumerates files, computes SHA512 checksums, and updates a local database; all changes are strictly confined to the database.
  * The database does not store file contents. It stores relative paths, checksums, and metadata such as size and timestamps (ctime/mtime).
  * The program does not open network sockets.
  * The program does not transmit data.
  * The program does not require privileged execution and does not use the SUID bit or other unsafe permission bits.
  * No functionality is provided for privilege escalation or other security violations.
* Performance is primarily limited by disk subsystem speed. Each file is read byte by byte, and its SHA512 checksum is computed.
* The program runs very fast thanks to SQLite and FTS libraries ([man 3 fts](https://man7.org/linux/man-pages/man3/fts.3.html)).
* Command-line argument parsing is handled via the ARGP library.
* Regular expression support is provided by PCRE2.
* The program is safe to use with an enormous number of files, directories, and deeply nested subdirectories. Thanks to the FTS library, recursion is avoided, preventing stack overflows even with extreme levels of nesting.
* Due to its compact and portable codebase, the program can be used even on specialized devices like NAS systems, embedded platforms, or IoT devices.
* The database contents created by **precizer** can be explored with [DB Browser for SQLite](https://sqlitebrowser.org).

## QUESTIONS & BUG REPORTS

* The `--help` option is designed to be as detailed as possible, specifically to assist users who may not have advanced technical knowledge.
* Author contact options:
  * [GitHub Discussions](https://github.com/precizer/precizer/discussions).
  * [Bug reports and feature requests](https://github.com/precizer/precizer/issues/new).

## CONTRIBUTING

Contributions are welcome. Start with [CONTRIBUTING](CONTRIBUTING.md) for workflow, dependencies, validation steps, and PR expectations. For open requests, check the [Issues](https://github.com/precizer/precizer/issues) list and pick a task that matches your interest and level of involvement.

## BUILD & INSTALLATION

### Packaging for Distributions

* The author has set up an automated build system using GitHub Workflows and will continue maintaining new versions.
* The author is **not** willing to personally package and maintain **precizer** for _all_ existing operating system distributions.
* If packaging for a specific distribution encounters major challenges adapting the code, the author can help with supporting the initiative and optimizing the program for the target distro or package manager. Contact details are in the [“Questions & Bug Reports”](#questions--bug-reports) section.

### Building with Docker

Building the program is already supported via Docker. Several tuned platforms are prepared and can be selected as the build distribution. Successfully tested distros:

* Almalinux
* Alpine
* Arch
* Debian
* Gentoo
* Rocky
* Ubuntu

Configuration details and installed libraries are listed in the corresponding Dockerfiles under `.docker/`.

Build targets use the form `docker-<distro>-<build>` (for example `debian` and `dynamic-production`).

```sh
make docker-gentoo-production
```

This builds a production binary using the Gentoo Docker container.

```sh
make docker-ubuntu-production
```

This builds the same `production` target using Ubuntu.

After the build completes, an executable `precizer` appears in the project directory (built inside the container). The main benefit of using Docker is that a full build toolchain, libraries, and their dependencies are not required on the host system; running Docker yields the binary. The next step is choosing the binary variant. When in doubt, `make portable` is a good starting point. All available build variants are described below.

### Manual Build

#### Preparation

```sh
git clone --depth=1 https://github.com/precizer/precizer.git
cd precizer
```

#### Portable binary

```sh
make portable
```

The result is a single statically linked, self-extracting compressed UPX ELF file with no dynamic dependencies. It contains the whole program and can be run on almost any modern Linux distribution. The file can be copied to any platform of the same architecture (x64/arm/etc).

The program is optimized for **maximum portability**.

Compilation and linking flags: `-static -O2 -mtune=generic`

Docker alternative:

```sh
make docker-ubuntu-portable
```

or replace `-ubuntu-` with any distro from the list above.

#### Single binary optimized for the local CPU

```sh
make production
```

The result is a statically linked, self-extracting compressed UPX ELF file tuned for the local CPU. It contains the whole program, can be run on the local machine, and will use the maximum available CPU features.

The program is optimized for **maximum possible performance on local hardware**.

Compilation and linking flags: `-static -O3 -march=native`

Docker alternative:

```sh
make docker-ubuntu-production
```

or replace `-ubuntu-` with any distro from the list above.

#### Dynamically linked binary optimized for the local CPU

```sh
make dynamic-production
```

The result is an ELF executable of about **50 kilobytes**. It is tuned for the local CPU and dynamically linked against libraries installed on the system; it is also self-extracting and UPX-compressed. It can be built and run on the local machine if libraries such as sqlite3, pcre2, argp and fts are installed.

The binary is optimized for **maximum performance and minimal size**.

Compilation flags: `-O3 -march=native`

Docker alternative:

```sh
make docker-ubuntu-dynamic-production
```

or replace `-ubuntu-` with any distro from the list above.

#### Tests

The test sets in the `tests/fixtures/` directory can be used to evaluate the program’s capabilities.

Test execution:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make tests
```

#### Installation

Just copy the resulting **precizer** executable to any location listed in the `$PATH` environment variable for quick invocation.

#### Build dependencies for specific OS

Install build and compile tools on Linux

#### Arch Linux

```sh
sudo pacman -S --noconfirm base-devel gcc-libs sqlite pcre2 upx
```

#### Ubuntu/Debian Linux

```sh
sudo apt -y install gcc make libpcre2-dev libsqlite3-dev upx-ucl
```

#### Alpine Linux

```sh
sudo apk add --update build-base pcre2-dev pcre2-static fts-dev argp-standalone sqlite-dev upx
```

#### Almalinux/Rocky Linux

```sh
sudo dnf -y install gcc make sqlite sqlite-devel glibc-devel pcre2 pcre2-devel upx pcre2-static glibc-static
```

#### Gentoo Linux

```sh
echo "dev-libs/libpcre2 static-libs" >> /etc/portage/package.use/libpcre2;
emerge dev-libs/libpcre2 app-arch/upx
```

#### Clean up

##### Remove all build artifacts

```sh
make purge
```

## USAGE EXAMPLES

### Example 1

Add files to two databases and compare them with each other:

```sh
precizer --progress --database=database1.db tests/fixtures/diffs/diff1

precizer --progress --database=database2.db tests/fixtures/diffs/diff2

precizer --compare database1.db database2.db
```

<sub>The comparison of database1.db and database2.db databases is starting…  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
Starting database file database2.db integrity check…  
Database database2.db has been verified and is in good condition  
**These files are no longer in the database1.db but still exist in the database2.db**  
path1/AAA/BCB/CCC/b.txt  
**These files are no longer in the database2.db but still exist in the database1.db**  
path2/AAA/ZAW/D/e/f/b_file.txt  
**The SHA512 checksums of these files do not match between database1.db and database2.db**  
2/AAA/BBB/CZC/a.txt  
3/AAA/BBB/CCC/a.txt  
4/AAA/BBB/CCC/a.txt  
path1/AAA/ZAW/D/e/f/b_file.txt  
path2/AAA/BCB/CCC/a.txt  
Comparison of database1.db and database2.db databases is complete  
The precizer completed its execution without any issues  
</sub>

In `--compare` mode, `--ignore` and `--include` limit the relative-path scope used to build the comparison report. If filters hide part of the differences, the equality messages apply only to the remaining filtered scope. Any path brought back with `--include` participates again in both the difference lists and the final summaries

### Example 2
Database Update

The previous example is run again. First attempt. Warning message.

```sh
precizer --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>The database database1.db was previously created and already contains data with files and their checksums. Use the `--update` option only when it is certain that the database needs to be updated and when file information (including changes, deletions, and additions) should be synchronized with the database.  
ERROR: The precizer process terminated unexpectedly due to an error  
</sub>

The **--update** parameter must be included. This parameter is required to protect the database from data loss caused by accidental execution.

```sh
precizer --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 45B, total items: 58, dirs: 46, files: 12, symlnks: 0  
**The database file database1.db has NOT been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

Make the following adjustments:

```sh
# Modify a file
echo -n "  " >> tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt

# Add a new file
touch tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/c.txt

# Remove a file
rm tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt

```

Run **precizer** again with the `--update` parameter:

```sh
precizer --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
The **--update** option has been used, so the information about files will be updated against the database database1.db  
File traversal started  
**These files have been added or changed and those changes will be reflected against the DB database1.db:**  
1/AAA/BCB/CCC/a.txt changed lsize & ctime & mtime rehashed  
1/AAA/BCB/CCC/c.txt added  
File traversal complete  
Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
**These files are no longer exist or ignored and will be deleted against the DB database1.db:**  
path2/AAA/ZAW/D/e/f/b_file.txt  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file database1.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

Change labels in output mean:
- `lsize` — logical file size in bytes (`st_size`)
- `asize` — allocated size on disk in bytes (`st_blocks * 512`)
- `ctime` — metadata/status change time
- `mtime` — file content modification time

Every time **precizer** runs, it traverses the file system and then checks whether a record for a specific file already exists in the database. In other words, the program prioritizes the current state of the file system on disk.

The directory traversal in **precizer** works similarly to `rsync` as it uses a similar algorithm.

It's important to note that **precizer** will not recalculate SHA512 checksums for files that are already recorded in the database, as long as their metadata remains unchanged (such as size and last access time, **atime**). If the `--watch-timestamps` argument is specified, the program will also consider the creation time (**ctime**) and modification time (**mtime**) in addition to the file size.

Any new, deleted, or modified files between application runs will be processed accordingly. All changes will be reflected in the database if the `--update` parameter is specified.

### Example 3

Using the `--silent` mode. When this mode is enabled, the program does not produce any output on the screen. This is useful when **precizer** is used in scripts.

An exception is `--compare`: with `--silent`, only compare results remain visible. Paths with differences are printed directly, and category headings are kept only when more than one compare category is active.

Add the **--silent** parameter to the previous example:

```sh
precizer --silent --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

As a result, nothing will be displayed on the screen.

### Example 4
Additional Information in `--verbose` mode. This mode can be useful for debugging.

Add the **--verbose** parameter to the previous example:

```sh
precizer --verbose --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>2025-01-25 09:55:59:820 src/parse_arguments.c:442:parse_arguments:Configuration: rational_logger_mode=VERBOSE  
paths=tests/fixtures/diffs/diff1; database=database1.db; db_file_name=database1.db; verbose=yes; maxdepth=-1; silent=no; force=no; update=yes; watch-timestamps=no; progress=yes; compare=no, db-drop-ignored=no, dry-run=no, check-level=FULL, rational_logger_mode=VERBOSE  
2025-01-25 09:55:59:820 src/parse_arguments.c:558:parse_arguments:Arguments parsed  
2025-01-25 09:55:59:820 src/detect_paths.c:025:detect_paths:Checking directory paths provided as arguments  
2025-01-25 09:55:59:820 src/file_availability.c:034:file_availability:Verify that the path tests/fixtures/diffs/diff1 exists  
2025-01-25 09:55:59:820 src/file_availability.c:053:file_availability:The path tests/fixtures/diffs/diff1 is exists and it is a directory  
2025-01-25 09:55:59:821 src/detect_paths.c:036:detect_paths:Paths detected  
2025-01-25 09:55:59:821 src/init_signals.c:034:init_signals:Set signal SIGUSR2 OK:pid:604770  
2025-01-25 09:55:59:821 src/init_signals.c:043:init_signals:Set signal SIGINT OK:pid:604770  
2025-01-25 09:55:59:821 src/init_signals.c:052:init_signals:Set signal SIGTERM OK:pid:604770  
2025-01-25 09:55:59:821 src/init_signals.c:055:init_signals:Signals initialized  
2025-01-25 09:55:59:821 src/determine_running_dir.c:018:determine_running_dir:Current directory: /tmp  
2025-01-25 09:55:59:821 src/db_determine_name.c:099:db_determine_name:Primary database file name: database1.db  
2025-01-25 09:55:59:821 src/db_determine_name.c:105:db_determine_name:Primary database file path: database1.db  
2025-01-25 09:55:59:821 src/db_determine_name.c:109:db_determine_name:DB name determined  
2025-01-25 09:55:59:821 src/file_availability.c:034:file_availability:Verify that the path . exists  
2025-01-25 09:55:59:821 src/file_availability.c:053:file_availability:The path . is exists and it is a directory  
2025-01-25 09:55:59:821 src/file_availability.c:034:file_availability:Verify that the path database1.db exists  
2025-01-25 09:55:59:821 src/file_availability.c:044:file_availability:The path database1.db is exists and it is a file  
2025-01-25 09:55:59:821 src/db_determine_mode.c:128:db_determine_mode:Final value for config->sqlite_open_flag: SQLITE_OPEN_READWRITE  
2025-01-25 09:55:59:821 src/db_determine_mode.c:129:db_determine_mode:Final value for config->db_initialize_tables: false  
2025-01-25 09:55:59:821 src/db_determine_mode.c:131:db_determine_mode:DB mode determined  
2025-01-25 09:55:59:821 src/db_test.c:061:db_test:Starting database file database1.db integrity check…  
2025-01-25 09:55:59:821 src/db_test.c:082:db_test:The database verification level has been set to FULL  
2025-01-25 09:55:59:821 src/db_test.c:126:db_test:Database database1.db has been verified and is in good condition  
2025-01-25 09:55:59:822 src/db_get_version.c:087:db_get_version:Version number 1 found in database  
2025-01-25 09:55:59:822 src/db_check_version.c:032:db_check_version:The database1.db database file is version 1  
2025-01-25 09:55:59:822 src/db_check_version.c:061:db_check_version:The database database1.db is on version 1 and does not require any upgrades  
2025-01-25 09:55:59:822 src/db_init.c:030:db_init:Successfully opened database database1.db  
2025-01-25 09:55:59:822 src/db_init.c:118:db_init:The primary database and tables have NOT been initialized  
2025-01-25 09:55:59:822 src/db_init.c:150:db_init:The primary database named database1.db is ready for operations  
2025-01-25 09:55:59:822 src/db_init.c:167:db_init:The in-memory runtime_paths_id database successfully attached to the primary database database1.db  
2025-01-25 09:55:59:822 src/db_init.c:174:db_init:Database initialization process completed  
2025-01-25 09:55:59:822 src/db_compare.c:136:db_compare:Database comparison mode is not enabled. Skipping comparison  
2025-01-25 09:55:59:822 src/db_contains_data.c:086:db_contains_data:The database database1.db has already been created previously  
2025-01-25 09:55:59:822 src/db_validate_paths.c:192:db_validate_paths:The paths written against the database and the paths passed as arguments are completely identical  
2025-01-25 09:55:59:822 src/file_list.c:143:file_list:File system traversal initiated to calculate file count and storage usage  
2025-01-25 09:55:59:823 src/file_list.c:038:show_status:Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
2025-01-25 09:55:59:825 src/db_get_version.c:087:db_get_version:Version number 1 found in database  
2025-01-25 09:55:59:825 src/db_consider_vacuum_primary.c:025:db_consider_vacuum_primary:No changes were made. The primary database doesn't require vacuuming  
2025-01-25 09:55:59:825 src/status_of_changes.c:049:status_of_changes:**The database file database1.db has NOT been modified since the program was launched**  
2025-01-25 09:55:59:825 src/exit_status.c:027:exit_status:The precizer completed its execution without any issues  
</sub>

### Example 5
Non-recursive traversal using the `--maxdepth` parameter

```sh
tree tests/fixtures/4

tests/fixtures/4
├── AAA
│   ├── BBB
│   │   ├── CCC
│   │   │   └── a.txt
│   │   └── uuu.txt
│   └── tttt.txt
└── sss.txt

3 directories, 4 files
```

The `--maxdepth=0` parameter completely disables recursion.

```sh
precizer --maxdepth=0 tests/fixtures/4
```

<sub>Primary database file name: myhost.db  
The path myhost.db doesn't exist or it is not a file  
The primary DB file not yet exists. Brand new database will be created  
Recursion depth limited to: 0  
File traversal started  
**These files will be added against the myhost.db database:**  
sss.txt  
File traversal complete  
Total size: 2B, total items: 5, dirs: 4, files: 1, symlnks: 0  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database myhost.db has been modified since the last check (files were added, removed, or updated)**  
The precizer completed its execution without any issues  
</sub>

### Example 6

Example of a Path to Ignore. To specify a pattern for ignoring files or directories, PCRE2 regular expressions can be used. **Note:** All paths in the regular expression must be specified as **relative**.

PCRE2 regular expressions can be tested and validated using https://regex101.com

To illustrate how a relative path looks, run a directory traversal without the `--ignore` option and check how the terminal displays the relative paths recorded in the database:

```sh
% tree -L 3 tests/fixtures/diffs

tests/fixtures/diffs
├── diff1
│   ├── 1
│   │   └── AAA
│   ├── 2
│   │   └── AAA
│   ├── 3
│   │   └── AAA
│   ├── 4
│   │   └── AAA
│   ├── path1
│   │   └── AAA
│   └── path2
│       └── AAA
└── diff2
    ├── 1
    │   └── AAA
    ├── 2
    │   └── AAA
    ├── 3
    │   └── AAA
    ├── 4
    │   └── AAA
    ├── path1
    │   └── AAA
    └── path2
        └── AAA

26 directories, 0 files
```

```sh
precizer --ignore="^diff1/1/.*" tests/fixtures/diffs
```

In this example, the initial traversal path is `./tests/fixtures/diffs`, and the generated ignore path is `./tests/fixtures/diffs/diff1/1/` along with all its subdirectories (`/*`).

<sub>Primary database file name: myhost.db  
The path myhost.db doesn't exist or it is not a file  
The primary DB file not yet exists. Brand new database will be created  
File traversal started  
**These files will be added against the myhost.db database:**  
diff1/1/AAA/BCB/CCC/a.txt **ignored & not added**  
diff1/1/AAA/ZAW/A/b/c/a_file.txt **ignored & not added**  
diff1/1/AAA/ZAW/D/e/f/b_file.txt **ignored & not added**  
diff1/2/AAA/BBB/CZC/a.txt  
diff1/3/AAA/BBB/CCC/a.txt  
diff1/4/AAA/BBB/CCC/a.txt  
diff1/path1/AAA/BCB/CCC/a.txt  
diff1/path1/AAA/ZAW/A/b/c/a_file.txt  
diff1/path1/AAA/ZAW/D/e/f/b_file.txt  
diff1/path2/AAA/BCB/CCC/a.txt  
diff1/path2/AAA/ZAW/A/b/c/a_file.txt  
diff1/path2/AAA/ZAW/D/e/f/b_file.txt  
diff2/1/AAA/BCB/CCC/a.txt  
diff2/1/AAA/ZAW/A/b/c/a_file.txt  
diff2/1/AAA/ZAW/D/e/f/b_file.txt  
diff2/2/AAA/BBB/CZC/a.txt  
diff2/3/AAA/BBB/CCC/a.txt  
diff2/4/AAA/BBB/CCC/a.txt  
diff2/path1/AAA/BCB/CCC/a.txt  
diff2/path1/AAA/BCB/CCC/b.txt  
diff2/path1/AAA/ZAW/A/b/c/a_file.txt  
diff2/path1/AAA/ZAW/D/e/f/b_file.txt  
diff2/path2/AAA/BCB/CCC/a.txt  
diff2/path2/AAA/ZAW/A/b/c/a_file.txt  
File traversal complete  
Total size: 97B, total items: 114, dirs: 90, files: 24, symlnks: 0  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database myhost.db has been modified since the last check (files were added, removed, or updated)**  
The precizer completed its execution without any issues  
Enjoy life!  
</sub>

Repeat the same example, but this time without the `--ignore` option to include the three previously ignored files:

```sh
precizer --update tests/fixtures/diffs
```

<sub>Primary database file name: myhost.db  
Starting database file myhost.db integrity check…  
Database myhost.db has been verified and is in good condition  
The **--update** option has been used, so the information about files will be updated against the database myhost.db  
File traversal started  
**These files have been added or changed and those changes will be reflected against the DB myhost.db:**  
diff1/1/AAA/BCB/CCC/a.txt add  
diff1/1/AAA/ZAW/A/b/c/a_file.txt add  
diff1/1/AAA/ZAW/D/e/f/b_file.txt add  
File traversal complete  
Total size: 97B, total items: 114, dirs: 90, files: 24, symlnks: 0  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file myhost.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

### Example 7

Continuation of the Previous Example [Example 6](#example-6).

Multiple regular expressions for ignoring files can be specified simultaneously by repeating the `--ignore` option.

The database will be cleaned of references to files matching the regular expressions provided via the `--ignore` arguments: `"diff1/1/.*"` and `"diff2/1/.*"`.

The `--db-drop-ignored` parameter must be explicitly specified to remove database entries for files that match the patterns passed through the `--ignore` option.

No changes were made to the file system, but the ignored files will be removed from the database.

```sh
# Update the database by removing entries for files that were marked as ignored:

precizer \
    --update \
    --db-drop-ignored \
    --ignore="^diff1/1/.*" \
    --ignore="^diff2/1/.*" \
    tests/fixtures/diffs
```

<sub>Primary database file name: myhost.db  
Starting database file myhost.db integrity check…  
Database myhost.db has been verified and is in good condition  
The **--update** option has been used, so the information about files will be deleted against the database myhost.db  
**These files are no longer exist or ignored and will be deleted against the DB myhost.db:**  
diff1/1/AAA/BCB/CCC/a.txt **clean ignored**  
diff1/1/AAA/ZAW/A/b/c/a_file.txt **clean ignored**  
diff1/1/AAA/ZAW/D/e/f/b_file.txt **clean ignored**  
diff2/1/AAA/BCB/CCC/a.txt **clean ignored**  
diff2/1/AAA/ZAW/A/b/c/a_file.txt **clean ignored**  
diff2/1/AAA/ZAW/D/e/f/b_file.txt **clean ignored**  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file myhost.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

### Example 8

Using `--ignore` together with `--include`

```sh

# Remove the old database and create a new one, then populate it with data:

rm -i "${HOST}.db"

precizer tests/fixtures/diffs
```

This variant uses regular expressions.

PCRE2 regular expressions for relative paths that need to be included. The specified relative paths will be included even if they were excluded using one or more `--ignore` parameters. Multiple regular expressions can be specified using `--include`.

PCRE2 regular expressions can be checked and tested using https://regex101.com

The DB will be cleaned of references to files matching the regular expressions provided in the `--ignore` arguments: `"^.*/path2/.*"` and `"diff2/.*"`, but paths matching the patterns in `--include` will remain in the database.

The `--db-drop-ignored` parameter must be specified additionally to remove references to files matching the regular expressions passed via the `--ignore` options from the database.

```sh
# Update the database, removing references to files that were marked as ignored,
# except for paths matching the --include patterns.

precizer --update \
	--progress \
	--ignore="^.*/path2/.*" \
	--ignore="^diff2/.*" \
	--include="^diff2/1/AAA/ZAW/A/b/c/.*" \
	--include="^diff2/path1/AAA/ZAW/.*" \
	--include="^diff1/path2/AAA/ZAW/A/b/c/a_file\..*" \
	--db-drop-ignored \
	tests/fixtures/diffs
```

<sub>Primary database file name: myhost.db  
Starting database file myhost.db integrity check…  
Database myhost.db has been verified and is in good condition  
The **--update** option has been used, so the information about files will be deleted against the database myhost.db  
**These files are no longer exist or ignored and will be deleted against the DB myhost.db:**  
diff1/path2/AAA/BCB/CCC/a.txt clean ignored  
diff1/path2/AAA/ZAW/A/b/c/a_file.txt clean ignored  
diff1/path2/AAA/ZAW/D/e/f/b_file.txt clean ignored  
diff2/1/AAA/BCB/CCC/a.txt clean ignored  
diff2/1/AAA/ZAW/D/e/f/b_file.txt clean ignored  
diff2/2/AAA/BBB/CZC/a.txt clean ignored  
diff2/3/AAA/BBB/CCC/a.txt clean ignored  
diff2/4/AAA/BBB/CCC/a.txt clean ignored  
diff2/path1/AAA/BCB/CCC/a.txt clean ignored  
diff2/path1/AAA/BCB/CCC/b.txt clean ignored  
diff2/path2/AAA/BCB/CCC/a.txt clean ignored  
diff2/path2/AAA/ZAW/A/b/c/a_file.txt clean ignored  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file myhost.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

#### The same filters in `--compare`

The same `--ignore` and `--include` combination also applies to database-to-database comparison. These filters do more than hide individual lines on screen: they define which relative paths remain inside the comparison report. As a result, the final summaries and equality messages are evaluated only against that filtered scope

```sh
# Continuing Example 1, bring back just one path from a hidden group of differences

precizer --compare \
	--ignore="^(?:2|3|4)/.*" \
	--ignore="^path1/.*" \
	--ignore="^path2/.*" \
	--include="^2/AAA/BBB/CZC/a\.txt$" \
	database1.db database2.db
```

<sub>The comparison of database1.db and database2.db databases is starting…  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
Starting database file database2.db integrity check…  
Database database2.db has been verified and is in good condition  
**The SHA512 checksums of these files do not match between database1.db and database2.db**  
2/AAA/BBB/CZC/a.txt  
Comparison of database1.db and database2.db databases is complete  
The precizer completed its execution without any issues  
</sub>

In this example, every difference under `2/`, `3/`, `4/`, `path1/`, and `path2/` is first pushed out of the comparison report, and then `--include` restores only `2/AAA/BBB/CZC/a.txt`. That leaves the report focused on that single restored path while the other hidden differences stay outside the final lists and summaries

### Example 9
Protecting immutable archives with `--lock-checksum`

Use `--lock-checksum` for archival folders whose contents must never be rewritten. It accepts PCRE2 regular expressions for **relative** paths (same format as `--ignore`). Paths matching any lock pattern are written to the database once. After that their checksums are not recalculated, even with `--update`. Any later size change, any timestamp drift when `--watch-timestamps` is enabled, the file disappearing from disk, a loss of read access, or an unexpected access-check failure is treated as data corruption and reported instead of updating the record. When a locked file disappears or becomes unavailable, its database row is kept so the violation remains visible in later runs. Lock protection takes priority over `--ignore`, `--db-drop-ignored`, and `--db-drop-inaccessible`: matching those filters does not silently drop the locked record from the database. The same protection also remains active when `--include` restores only part of an ignored subtree. You can provide multiple patterns by repeating the option.

```sh
precizer \
  --lock-checksum="^archive/2024/.*" \
  --lock-checksum="^snapshots/monthly/.*" \
  /mnt/storage
```

On subsequent runs, the same lock patterns must be preserved while refreshing the database:

```sh
precizer \
  --update \
  --lock-checksum="^archive/2024/.*" \
  --lock-checksum="^snapshots/monthly/.*" \
  /mnt/storage
```

Files outside the lock patterns follow normal update rules. For entries locked via `--lock-checksum`, any drift becomes visible immediately and `precizer` exits with a non-zero status, which can be used in scripts.

### Example 10
Deep verification of locked data with `--rehash-locked`

The `--rehash-locked` option works only together with `--lock-checksum`. When it is enabled, every file that matches a lock pattern and already exists in the database is read again, its SHA512 checksum is recomputed, and the result is compared against the stored checksum. This provides an explicit integrity sweep for immutable archives at the cost of extra disk I/O. The option ignores whether `--watch-timestamps` is enabled or not. If the recalculated checksum and recorded size match, the file is considered consistent; if its timestamps on disk differ from the database, the ctime/mtime fields in the database are updated with the new values. If a locked path also matches `--ignore`, `--rehash-locked` still traverses and verifies that path instead of suppressing the check.

```sh
precizer --update \
  --lock-checksum="^archive/2024/.*" \
  --rehash-locked \
  /mnt/storage
```

The following cases illustrate how `--lock-checksum`, `--watch-timestamps`, and `--rehash-locked` interact:

1. **File size mismatch.** If the size stored in the database differs from the on-disk size, the file is flagged as a “locked checksum violation” regardless of `--watch-timestamps` and `--rehash-locked`. Rehashing a file with a different size is meaningless because the checksum cannot match anyway.
2. **File size matches; neither `--watch-timestamps` nor `--rehash-locked` is used.** Other values, such as SHA512 and timestamps, are not considered; the file is treated as fully consistent and `precizer` finishes with the `SUCCESS` status.
3. **Size and timestamps match; `--watch-timestamps` is enabled and `--rehash-locked` is omitted.** The file is treated as fully consistent, does not appear in the output, and `precizer` finishes with the `SUCCESS` status.
4. **Size matches, timestamps differ; `--watch-timestamps` is enabled and `--rehash-locked` is omitted.** The file is flagged as a “locked checksum violation” only due to timestamp drift, and `precizer` finishes with the `WARNING` status.
5. **Size matches; `--rehash-locked` is enabled.** Only the checksum and the size stored in the database matter. If both match, the file is considered consistent. If the on-disk timestamps changed, the new ctime/mtime values are saved to the database regardless of whether `--watch-timestamps` was used.

Detailed examples of behavior in difficult situations are collected in test No. 30 (`tests/src/test0030.c`). It covers deleted locked files, lost access, access-check failures, and interactions with `--ignore`, `--include`, `--db-drop-ignored`, and `--db-drop-inaccessible`.

A practical workflow is to run a quick daily scan without `--rehash-locked` (and even without `--watch-timestamps` if timestamp drift is acceptable) to keep the database synchronized, then schedule a less frequent deep audit with `--rehash-locked` to force checksum-level verification of the frozen data set.

### Example 11
Dropping inaccessible records with `--db-drop-inaccessible`

By default, when a file is inaccessible because of permission errors, its database record is preserved during `--update` to prevent accidental data loss. Dropping such records requires `--db-drop-inaccessible`:

```sh
precizer --update --db-drop-inaccessible /mnt/storage
```

<sub>drop due to inaccessible archive/secret.bin</sub>

Important: `--db-drop-inaccessible` is not about files that are gone. It is about files that cannot be read right now, or files whose access state cannot be checked reliably. For example, permissions, ownership, ACLs, or the filesystem itself may temporarily prevent access. By default, these records stay in the database so a temporary access problem does not erase useful history. If you are sure those records should be removed, add `--db-drop-inaccessible`.

If a file, or one of the directories in its path, is not visible in the filesystem at all, `precizer` treats a regular record not protected by `--lock-checksum` as deleted and removes it from the database during `--update` without any extra option. The program cannot tell whether this is a real deletion or a mount point that exists but currently shows an empty directory because the expected volume was not mounted. Before updating a database for external, network, or removable volumes, first make sure the expected volume is actually mounted. For important archive paths, use `--lock-checksum`: then a missing or unavailable file is reported as a warning, and its database record is preserved.

## TROUBLESHOOTING

### Slow file walk, slow checksums, slow database writes ("everything is slow")

To pinpoint the bottleneck, try running `precizer` in `--dry-run` or `--dry-run=with-checksums` mode.

`--dry-run` recursively walks the **file system**. In this mode, nothing happens except directory tree traversal. You can add `--progress` to also count total bytes and files, but no database writes will occur. This mode helps validate file system accessibility and, to a degree, the underlying hardware. If it is slow even with `--dry-run`, the root cause is unlikely to be `precizer` itself.

`--dry-run=with-checksums` differs from `--dry-run` only in that every encountered file is fully read (byte-by-byte) and a checksum is computed. This is significantly more resource-intensive and is close to the program's real workload, but it still does not write to the database. With `--progress` enabled, `precizer` also prints how many bytes were hashed and the average hashing throughput in B/s. That number can be compared against third-party benchmarks to help spot the bottleneck.

It is also possible that `--dry-run=with-checksums` is fast, but real runs (non-dry-run) slow down noticeably, especially when many records are being added or changed. In that case, check the file system that stores the database file — the issue may be there.

`precizer` opens the SQLite database using settings that favor keeping already-written data safe and resisting database corruption as much as possible. The tradeoff is more disk I/O and more file system syncs to the underlying block device. In practice, SQLite is very fast and usually not the weakest link. However, a compressed, networked, or simply slow file system can materially affect overall performance.

For example, during mass inserts `precizer` waits for the file record to be written and the transaction to be committed before moving on to the next file. As a quick test, try placing the `.db` file temporarily on a fast medium (for example, `tmpfs`) — this can both improve overall performance and help confirm where the bottleneck is.

### If everything is still slow in every mode, check:

#### File system integrity

Run file system checks on the volume being scanned (where checksums are captured) and on the volume where the `.db` file is stored and updated. Performance drops can be caused by logical file system issues.

#### Hardware

##### Disk throughput

Files are read fully, byte-by-byte. Disk subsystem throughput is directly reflected in `precizer` speed. The program works at the file system level (a higher abstraction), not directly with raw block devices, so file system health matters.

Performance can vary significantly depending on the file system. For example, reading files from an NFS mount may be limited by network throughput and behave very differently from reading from a local NVMe file system with `atime` disabled (`atime` is the access time timestamp). Use third-party tools to benchmark the storage subsystem.

##### CPU performance

Checksum computation is pure math and can be CPU-intensive. Modern CPUs usually handle it easily, but it is worth ensuring performance is not degraded by shared vCPU resources in containerized or virtualized setups. Use monitoring and benchmarks to validate CPU performance.

### Bottleneck triage matrix

| Step | Mode/command                      | What it measures                                                      | If it is slow here                                                        | Next steps                                                                                         |
| ---- | --------------------------------- | --------------------------------------------------------------------- | ------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| 1    | precizer --dry-run                | File system accessibility, directory walk speed, baseline I/O         | Most likely outside `precizer`: file system/disk/network/system load      | Check storage subsystem, mount status, and overall system load                                     |
| 2    | precizer --dry-run=with-checksums | Real read + checksum compute speed (no DB writes)                     | Bottleneck: data reads (disk/network/file system) or CPU (more rarely)    | Check disk/network throughput, file system settings, and CPU resource limits (VMs/containers)      |
| 3    | Normal run (not dry-run)          | Impact of SQLite writes and transactions                              | Often the file system hosting `.db` is the issue (slow/network/compressed)| Check the `.db` file system; try moving `.db` temporarily to a faster medium or `tmpfs`            |

## ALTERNATIVES

Alternatives to **precizer** with open architectures and source code. More alternatives (kept up to date): [www.alternativeto.net](https://alternativeto.net/software/precizer-verify-file-checksums-at-scale/)

* **AIDE** — [aide.github.io](https://aide.github.io/)
  * Platforms/architectures: Linux/*BSD/macOS (x86_64, arm64, etc.)
  * Written in: C, actively maintained
    * Supports selecting/combining different hash algorithms and controlling more non-hash attributes (ACL/xattr/SELinux, etc.) at the rule level.
    * Snapshot/database uses a text format (optionally gzip), not SQLite: less suited for complex queries/analytics over the database.
    * No resumption of interrupted hashing inside a large file: a re-scan typically starts over from scratch.

* **Samhain** — [www.la-samhna.de/samhain](https://www.la-samhna.de/samhain/)
  * Platforms/architectures: Linux/Unix/POSIX, Windows (x86_64, arm64, etc.)
  * Written in: C, actively maintained
    * Built-in agent + central server model (centralized collection/control), which goes beyond "one local database".
    * Emphasis on tamper-resistance (signatures/cryptographic protection of some artifacts).
    * Significantly more complex deployment/maintenance (agents/server/keys) if all you need is a quick comparison of two trees via SQLite.

* **OSSEC** — [www.ossec.net](https://www.ossec.net/)
  * Platforms/architectures: Linux, Windows, macOS, *BSD (x86_64, arm64, etc.)
  * Written in: C, actively maintained
    * Event-driven/agent-based HIDS platform: real-time alerts, rules, correlation, response — a different class of tasks than "snapshot + diff".
    * Centralized architecture out of the box.
    * Not designed for storing a file snapshot specifically in SQLite or for resuming interrupted checksum computation inside a large file.

* **Open Source Tripwire** — [github.com/Tripwire/tripwire-open-source](https://github.com/Tripwire/tripwire-open-source)
  * Platforms/architectures: POSIX-like OSes (Linux/macOS/*BSD/Solaris, etc.), Windows via Cygwin (x86_64, arm64, etc.)
  * Written in: C++, ended in 2018
    * Tripwire as an open source project ended in 2018 and has not been developed since then.
    * precizer is more of a tool for integrity checking of large file trees, specifically designed for fast comparison. It is noticeably easier to get started with and can solve the comparison task in three commands. Tripwire addresses similar needs, but it is a classic file integrity monitoring tool. It requires a policy file, configs, database, and digital signatures for all of that.

* **integrit** — [github.com/integrit/integrit](https://github.com/integrit/integrit)
  * Platforms/architectures: Linux/*BSD (x86_64, arm64, etc.)
  * Written in: C, last changes 2 years ago
    * Minimalism and low external library dependencies (if a "small utility" matters more than a SQL data model).
    * Does not use SQLite and, as a result, is less suited for heavy queries/comparisons/analytics over snapshots.

* **mtree (NetBSD mtree)** — [man.netbsd.org/mtree.8](https://man.netbsd.org/mtree.8)
  * Platforms/architectures: *BSD, Linux (x86_64, arm64, etc.)
  * Written in: C, last changes 18 years ago
    * Very old codebase (effectively frozen).
    * mtree and go-mtree use a text-based snapshot format that is not intended for indexing or high-speed comparison. For workloads involving millions or hundreds of millions of files, precizer provides better performance.

* **hashdeep (md5deep/sha*deep)** — [github.com/jessek/hashdeep](https://github.com/jessek/hashdeep)
  * Platforms/architectures: Linux, Windows, macOS (x86_64, arm64, etc.)
  * Written in: C++/C, last changes 9 years ago
    * Choice of hash families/output formats. Convenient when output must conform to external requirements/standards.
    * No SQLite snapshot as a product: results are primarily report files/listings, not a database for fast comparisons.

* **hashit** — [github.com/boyter/hashit](https://github.com/boyter/hashit)
  * Platforms/architectures: Linux, Windows, macOS (x86_64, arm64, etc.)
  * Written in: Go, actively maintained
    * Can compute multiple different hashes for one file in a single pass.
    * No SQLite snapshot and no database update mechanisms.

* **RHash** — [github.com/rhash/RHash](https://github.com/rhash/RHash)
  * Platforms/architectures: Linux, Windows, macOS, *BSD (x86_64, arm64, etc.)
  * Written in: C, actively maintained
    * Very wide range of algorithms/output formats (including magnet links, etc.) when compatibility with external ecosystems is required.
    * Does not maintain a SQLite snapshot of a directory tree as a primary entity (more "compute/verify hashes" than "maintain a snapshot database").

* **rsync** — [rsync.samba.org](https://rsync.samba.org/)
  * Platforms/architectures: Linux, *BSD, macOS (x86_64, arm64, etc.), Windows via Cygwin/MSYS2
  * Written in: C, actively maintained
    * Combines transfer/synchronization with verification: divergences can be corrected immediately rather than just detected.
    * Checksums are not persisted between runs: after interruptions or re-checks the computation starts over from scratch.

* **rclone** — [rclone.org](https://rclone.org/)
  * Platforms/architectures: Linux, Windows, macOS, *BSD (amd64/arm/arm64, etc.)
  * Written in: Go, actively maintained
    * Oriented toward remote/cloud storage: S3/Drive/etc.
    * Does not maintain a local SQLite snapshot of a directory tree for fast offline A↔B comparisons.
    * Integrity checking often depends on the capabilities of the specific backend (which hashes/metadata it exposes).

* **QuickHash GUI** — [www.quickhash-gui.org](https://www.quickhash-gui.org/)
  * Platforms/architectures: Linux, Windows, macOS (x86_64; macOS arm64)
  * Written in: FreePascal (Lazarus), last changes 2 years ago
    * GUI-only tool oriented toward various media/artifacts (e.g., forensics scenarios) where CLI snapshots are not always convenient.
    * Does not use a SQLite directory-tree snapshot as the core of its workflow.

* **restic** — [restic.net](https://restic.net/)
  * Platforms/architectures: Linux, Windows, macOS, *BSD (x86_64, arm64, etc.)
  * Written in: Go, actively maintained
    * Backup repository with snapshots, deduplication, and encryption — if the goal is "store history and transfer it", this is more powerful than simply comparing two trees.
    * Different approach: chunk/dedup model instead of "SQLite snapshot + diff"; may be overkill for the pure A↔B comparison task.
    * Does not provide a straightforward model for comparing two local directory trees through a single SQL database.

## AUTHOR
Software author: [Dennis V. Razumovsky](https://github.com/dennisrazumovsky)

## COPYING
This program is distributed under the GNU General Public License v3.0 (GPLv3) as provided in the top-level `COPYING` file. The author is not responsible for any use of the source code or the entire program. Anyone who uses the code or the program uses it at their own risk and responsibility

### Usage Restrictions within Territory Under the Ruscist Terrorist Regime, Where Power Has Been Seized by an Authoritarian Dictatorship

- Permitted: strictly personal, non-commercial use by private individuals.
- Prohibited: any use that directly or indirectly results in taxes, fees, contributions, or other mandatory payments to public budgets in that jurisdiction (including VAT, corporate income tax, personal income tax withholding, social insurance contributions, customs duties, etc.).
- Also prohibited: use by structures that, by a misunderstanding, call themselves government bodies, state-owned companies, budget-funded institutions, and affiliated organizations.
- Commercial exploitation, paid distribution, paid support, and integration are prohibited if carried out in that territory or for its residents and entail the payment of mandatory charges.
- The restriction applies to the program itself and to its source code, in whole or in part.
- Purpose: to prevent direct and indirect financing of the war in Ukraine.
