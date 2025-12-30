[<img src="/img/i18n-icon.svg"> Link to the Russian language README page](/ru/)

# Precizer — verify file checksums at scale
A Tiny, High-Performance File Integrity and Comparison Tool

“A truly great application will always fit on a floppy disk. Hopefully, someone out there still remembers what those were… But it’s not about the floppies, it’s about quality software!”<sup>©</sup> :-D

<p><a class="btn" href="https://github.com/precizer/precizer/releases/latest">Download (stable release)</a></p>

<p width="100%" height="100%"><img width="20%" src="/img/micrometer_0.svg"></p>

<a href="/.code_coverage_report/"><img src="/img/unit-coverage.svg" height="20" alt="Unit Tests Code Coverage" /><br><img src="/img/system-coverage.svg" height="20" alt="System Tests Code Coverage"/></a>

[![Precizer build & testing](https://github.com/precizer/precizer/actions/workflows/precizer.yml/badge.svg)](https://github.com/precizer/precizer/actions/workflows/precizer.yml)

## TL;DR

### Overview

**precizer** is a lightweight and blazing-fast command-line application written entirely in pure C. It is designed for file integrity verification and comparison, making it particularly useful for checking synchronization results. The program recursively traverses directories, generating a database of files and their checksums for quick and efficient comparisons.

Built for both embedded platforms and large-scale clustered mainframes, **precizer** helps detect synchronization errors by comparing files and their checksums across different sources. It can also be used to analyze historical changes by comparing databases generated at different points in time from the same source.

### Basic Example

Consider a scenario where two machines have large mounted volumes at `/mnt1` and `/mnt2`, respectively, containing identical data. The goal is to verify, byte by byte, whether the contents are truly identical or if discrepancies exist.

1. Run **precizer** on the first machine (e.g., hostname `host1`):

```sh
precizer --progress /mnt1
```

This command recursively traverses all directories under `/mnt1`, creating a database file `host1.db` in the current directory. The `--progress` flag provides real-time progress updates, displaying the total traversed space and the number of processed files.

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

## TECHNICAL DETAILS

Consider a scenario where a primary storage system has a backup copy. For example, this could be a data center storage and its *Disaster Recovery* copy.

Synchronization from the primary storage to the backup occurs periodically, but due to the *massive data volumes*, synchronization is most likely not performed byte-by-byte but rather by detecting *metadata changes* within the file system. In such cases, *file size* and *modification time* are taken into account, but the actual content is *not verified byte by byte*.

This approach makes sense because the primary data center and the *Disaster Recovery* site usually have *high-speed communication channels*, but a full byte-by-byte synchronization would take an *unreasonably long time*.

Tools like `rsync` allow both types of synchronization — *metadata-based* and *byte-by-byte* — but they have one *major drawback*: *state is not preserved between sessions*.

Let’s analyze this issue with the following scenario:

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
* The checksum calculations rely on a reliable and fast SHA512 algorithm, which completely eliminates collisions even when analyzing a single massive file. If there are two identical large files differing by just one byte, SHA512 will detect it, and their checksums will be different—something that cannot be guaranteed with simpler hash functions like SHA1 or CRC32.
* The algorithms in precizer are designed to make it easy to keep the database up to date without having to recalculate everything from scratch. Simply run the program with the `--update` parameter, and new files will be added to the database, while entries for deleted files will be removed. If a file has been modified and its size has changed, its SHA512 checksum will be recalculated and updated in the database.
* There is an option to consider not only the file size when updating the database but also the file’s creation or modification timestamps. This means that any change in file metadata will trigger an SHA512 checksum recalculation and update in the database. For example, if a file’s ctime changes but its size remains the same, the checksum will NOT be recalculated if only the `--update` parameter is used. To force checksum recalculation for such files `--watch-timestamps` should be added. This option is disabled by default because ctime (like mtime) can change frequently due to commands like `chmod` or `chown`, even when the file’s content remains the same.
* precizer can be used as a security monitoring tool, detecting unauthorized file modifications where contents might have changed while metadata remains untouched.
* The program never modifies, deletes, moves, or copies any files or directories it processes. All it does is list files, compute their checksums, and update them in the database. All changes are strictly confined to the database.
* Performance is primarily limited by disk subsystem speed. Each file is read byte by byte, and its SHA512 checksum is computed.
* The program runs very fast thanks to SQLite and FTS libraries ([man 3 fts](https://man7.org/linux/man-pages/man3/fts.3.html)).
* Command-line argument parsing is handled via the ARGP library.
* Regular expression support is provided by PCRE2.
* The program is safe to use with an enormous number of files, directories, and deeply nested subdirectories. Thanks to the FTS library, recursion is avoided, preventing stack overflows even with extreme levels of nesting.
* Due to its compact and portable codebase, the program can be used even on specialized devices like NAS systems, embedded platforms, or IoT devices.
* Use [DB Browser for SQLite](https://sqlitebrowser.org) if you want to explore the contents of the database created by **precizer**.

## QUESTIONS & BUG REPORTS

* The `--help` option is designed to be as detailed as possible, specifically to assist users who may not have advanced technical knowledge.
* You can reach out to the author via:
  * [GitHub Discussions](https://github.com/precizer/precizer/discussions).
  * You can also [report a bug on GitHub](https://github.com/precizer/precizer/issues/new).
* If you run into issues while using the program, feel free to ask a question on [stackoverflow.com](https://stackoverflow.com) using the **precizer** tag. The author actively monitors such questions and will be happy to help with troubleshooting any problems.

## BUILD & INSTALLATION

### Prebuilt Portable Version

A fully ready-to-use version [can be downloaded here](https://github.com/precizer/precizer/releases).

#### Technical Details of the Portable Build

The prebuilt version is a statically linked ELF binary that can be run immediately on nearly any x64 Linux distribution.

The binary is automatically built using GitHub's CI/CD pipeline, then compressed with [UPX (an executable file packer)](https://upx.github.io).

The final self-extracting compressed binary is then placed inside a zip archive for easier downloading.

To use it, simply extract the zip file and run the executable.

### Packaging for Distributions

* The author has set up an automated build system using GitHub Workflows and will continue maintaining new versions.
* However, the author is **not** willing to personally package and maintain **precizer** for _all_ existing operating system distributions.
* If you are eager to create a package for a specific distribution but encounter significant challenges adapting the code, the author will gladly provide assistance in optimizing the program for that distribution or package manager. Contact details can be found in the [“Questions & Bug Reports”](#questions--bug-reports) section.

### Manual Build

The build process produces a statically linked ELF binary with no external dependencies. This self-contained executable can run on nearly any modern Linux distribution.

Most required libraries are embedded into the binary, and by default, the program is built as a static executable. This approach enhances portability and eliminates dependency issues. Thanks to this setup, compiling the program on most modern platforms is straightforward — just follow these steps:

1. Install build and compile tools on Linux

#### Arch Linux

```sh
sudo pacman -S --noconfirm base-devel
```

#### Debian/Ubuntu Linux

```sh
sudo apt -y install build-essential
```

#### Alpine Linux

```sh
sudo apk add --update build-base fts-dev argp-standalone
```

#### Gentoo Linux

```sh
sudo emerge --ask dev-libs/libpcre2

```

2. Get the source code

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
```

3. Build the project

```sh
make
```

4. Copy the compiled **precizer** binary to any directory listed in the system's `$PATH` to enable quick execution.

5. Clean up

```sh
# Remove build artifacts
make clean

# Remove all build files, including compiled libraries
make clean-all
```

6. Update

```sh
git pull
make

# Then proceed to step 4.
```

### Building a Portable Version

Repeat steps 1. and 2. Instead of step 3, run:

```sh
make portable
```

### Building with Docker

If you prefer not to install additional packages on your system, you can use a preconfigured Docker-based build environment.

To build the project, all you need is a working installation of Docker.

Running the simple `make docker` command:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make docker
```

will generate a compiled `precizer` binary in the current directory. You can either run it from there or move it to a directory listed in `$PATH`.

If `make` is not installed, you can still build the application inside a container with these commands:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
docker build -t precizer .
docker create --name precizer precizer
docker cp precizer:/precizer/precizer precizer
docker rm -f precizer
```

This will produce a statically linked ELF binary in the current directory.

If you run into compatibility issues with the compiled binary across different systems, you can try increasing its portability:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make docker-portable
```
or

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
docker build --build-arg OS=ubuntu:18.04 --build-arg BUILD=portable -t precizer .
docker create --name precizer precizer
docker cp precizer:/precizer/precizer precizer
docker rm -f precizer
```

## USAGE EXAMPLES

### Running Tests

To evaluate the program’s capabilities, you can use the test sets available in the `tests/examples/` directory within the source code.

Run tests with the following commands:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make debug
cd tests/
make debug
./testitall
```

### Example 1

Add files to two databases and compare them with each other:

```sh
precizer --progress --database=database1.db tests/examples/diffs/diff1

precizer --progress --database=database2.db tests/examples/diffs/diff2

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

### Example 2
Database Update

Let’s run the previous example again. First attempt. Warning message.

```sh
precizer --progress --database=database1.db tests/examples/diffs/diff1
```

<sub>The database database1.db was previously created and already contains data with files and their checksums. Use the `--update` option only when you are certain that the database needs to be updated and when file information (including changes, deletions, and additions) should be synchronized with the database.  
ERROR: The precizer process terminated unexpectedly due to an error  
</sub>

The **--update** parameter must be included. This parameter is required to protect the database from data loss caused by accidental execution.

```sh
precizer --update --progress --database=database1.db tests/examples/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 45B, total items: 58, dirs: 46, files: 12, symlnks: 0  
**The database file database1.db has NOT been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

Now let's make some adjustments:

```sh
# Modify a file
echo -n "  " >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt

# Add a new file
touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt

# Remove a file
rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt

```

And run **precizer** again, this time with the `--update` parameter:

```sh
precizer --update --progress --database=database1.db tests/examples/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
The **--update** option has been used, so the information about files will be updated against the database database1.db  
File traversal started  
**These files have been added or changed and those changes will be reflected against the DB database1.db:**  
1/AAA/BCB/CCC/a.txt changed size & ctime & mtime rehashed  
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

Every time **precizer** runs, it traverses the file system and then checks whether a record for a specific file already exists in the database. In other words, the program prioritizes the current state of the file system on disk.

The directory traversal in **precizer** works similarly to `rsync` as it uses a similar algorithm.

It's important to note that **precizer** will not recalculate SHA512 checksums for files that are already recorded in the database, as long as their metadata remains unchanged (such as size and last access time, **atime**). If the `--watch-timestamps` argument is specified, the program will also consider the creation time (**ctime**) and modification time (**mtime**) in addition to the file size.

Any new, deleted, or modified files between application runs will be processed accordingly. All changes will be reflected in the database if the `--update` parameter is specified.

### Example 3

Using the `--silent` mode. When this mode is enabled, the program does not produce any output on the screen. This is useful when **precizer** is used in scripts.

Let's add the **--silent** parameter to the previous example:

```sh
precizer --silent --update --progress --database=database1.db tests/examples/diffs/diff1
```

As a result, nothing will be displayed on the screen.

### Example 4
Additional Information in `--verbose` mode. This mode can be useful for debugging.

Let's add the **--verbose** parameter to the previous example:

```sh
precizer --verbose --update --progress --database=database1.db tests/examples/diffs/diff1
```

<sub>2025-01-25 09:55:59:820 src/parse_arguments.c:442:parse_arguments:Configuration: rational_logger_mode=VERBOSE  
paths=tests/examples/diffs/diff1; database=database1.db; db_file_name=database1.db; verbose=yes; maxdepth=-1; silent=no; force=no; update=yes; watch-timestamps=no; progress=yes; compare=no, db-clean-ignored=no, dry-run=no, check-level=FULL, rational_logger_mode=VERBOSE  
2025-01-25 09:55:59:820 src/parse_arguments.c:558:parse_arguments:Arguments parsed  
2025-01-25 09:55:59:820 src/detect_paths.c:025:detect_paths:Checking directory paths provided as arguments  
2025-01-25 09:55:59:820 src/file_availability.c:034:file_availability:Verify that the path tests/examples/diffs/diff1 exists  
2025-01-25 09:55:59:820 src/file_availability.c:053:file_availability:The path tests/examples/diffs/diff1 is exists and it is a directory  
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
tree tests/examples/4

tests/examples/4
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
precizer --maxdepth=0 tests/examples/4
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

Example of a Path to Ignore. To specify a pattern for ignoring files or directories, you can use PCRE2 regular expressions. **Note:** All paths in the regular expression must be specified as **relative**.

You can test and validate PCRE2 regular expressions using [https://regex101.com/](https://regex101.com/).

To understand how a relative path looks, simply run a directory traversal without the `--ignore` option and check how the terminal displays the relative paths recorded in the database:

```sh
% tree -L 3 tests/examples/diffs

tests/examples/diffs
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
precizer --ignore="^diff1/1/.*" tests/examples/diffs
```

In this example, the initial traversal path is `./tests/examples/diffs`, and the generated ignore path is `./tests/examples/diffs/diff1/1/` along with all its subdirectories (`/*`).

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
Enjoy your life!  
</sub>

Let's repeat the same example, but this time without the `--ignore` option to include the three previously ignored files:

```sh
precizer --update tests/examples/diffs
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

Multiple regular expressions for ignoring files can be specified simultaneously using the `--ignore` option.

The database will be cleaned of references to files matching the regular expressions provided via the `--ignore` arguments: `"diff1/1/.*"` and `"diff2/1/.*"`.

The `--db-clean-ignored` parameter must be explicitly specified to remove database entries for files that match the patterns passed through the `--ignore` option.

No changes were made to the file system, but the ignored files will be removed from the database.

```sh
# Update the database by removing entries for files that were marked as ignored:

precizer \
    --update \
    --db-clean-ignored \
    --ignore="^diff1/1/.*" \
    --ignore="^diff2/1/.*" \
    tests/examples/diffs
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

precizer tests/examples/diffs
```

Let's complicate things by using regular expressions.

PCRE2 regular expressions for relative paths that need to be included. The specified relative paths will be included even if they were excluded using one or more `--ignore` parameters. Multiple regular expressions can be specified using `--include`.

To check and test PCRE2 regular expressions, you can use [https://regex101.com/](https://regex101.com/).

The DB will be cleaned of references to files matching the regular expressions provided in the `--ignore` arguments: `"^.*/path2/.*"` and `"diff2/.*"`, but paths matching the patterns in `--include` will remain in the database.

The `--db-clean-ignored` parameter must be specified additionally to remove references to files matching the regular expressions passed via the `--ignore` options from the database.

```sh
# Update the database, removing references to files that were marked as ignored,
# except for paths matching the --include patterns.

precizer --update --db-clean-ignored \
	--ignore="^.*/path2/.*" \
	--ignore="^diff2/.*" \
	--include="^diff2/1/AAA/ZAW/A/b/c/.*" \
	--include="^diff2/path1/AAA/ZAW/.*" \
	tests/examples/diffs
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

### Example 9
Protecting immutable archives with `--lock-checksum`

Use `--lock-checksum` for archival folders whose contents must never be rewritten. It accepts PCRE2 regular expressions for **relative** paths (same format as `--ignore`). Paths matching any lock pattern are written to the database once. After that their checksums are not recalculated, even with `--update`. Any later change in size, or in timestamps when `--watch-timestamps` is enabled, is treated as data corruption and reported instead of updating the record. You can provide multiple patterns by repeating the option.

```sh
precizer \
  --lock-checksum="^archive/2024/.*" \
  --lock-checksum="^snapshots/monthly/.*" \
  /mnt/storage
```

On subsequent runs, keep the same lock patterns while refreshing the database:

```sh
precizer --update --lock-checksum="^archive/2024/.*" /mnt/storage
```

Files outside the lock patterns follow normal update rules. For entries locked via `--lock-checksum`, any drift becomes visible immediately.

### Example 10
Deep verification of locked data with `--rehash-locked`

The `--rehash-locked` option works only together with `--lock-checksum`. When it is enabled, every file that matches a lock pattern and already exists in the database is read again, its SHA512 checksum is recomputed, and the result is compared against the stored checksum. This provides an explicit integrity sweep for immutable archives at the cost of extra disk I/O. The option ignores whether `--watch-timestamps` is enabled or not. If the recalculated checksum and recorded size match, the file is considered consistent; if its timestamps on disk differ from the database, the ctime/mtime fields in the database are updated with the new values.

```sh
precizer --update \
  --lock-checksum="^archive/2024/.*" \
  --rehash-locked \
  /mnt/storage
```

To illustrate how `--watch-timestamps` and `--rehash-locked` interact, consider the following cases:

1. **File size mismatch.** When the size stored in the database differs from the on-disk size, the file is reported as having a corrupted checksum regardless of `--watch-timestamps` or `--rehash-locked`. Rehashing a file with a different size is meaningless because the checksum cannot match anyway.
2. **Size and timestamps match; `--watch-timestamps` enabled.** The file is fully consistent; it is not flagged and does not appear in the change report. `precizer` finishes with the `SUCCESS` status.
3. **Size matches, timestamps differ; `--watch-timestamps` enabled while `--rehash-locked` is omitted.** The file is flagged as “checksum violated” and `precizer` finishes with the `WARNING` status.
4. **Size matches, timestamps differ; neither `--watch-timestamps` nor `--rehash-locked` is used.** The file is **not** flagged, and the program exits with `SUCCESS`.
5. **Both `--watch-timestamps` and `--rehash-locked` are enabled.** Only the checksum and the size stored in the database matter. If both match, the file remains consistent and no warning is produced. If the on-disk timestamps changed, the new values are saved to the database even though the checksum stayed the same.

A practical workflow is to run a quick daily scan without `--rehash-locked` (and even without `--watch-timestamps` if timestamp drift is acceptable) to keep the database synchronized, then schedule a less frequent deep audit with `--rehash-locked` to force checksum-level verification of the frozen data set.

## AUTHOR
Software author: [Dennis V. Razumovsky](https://github.com/dennisrazumovsky)

## LICENSE
This program is distributed under the [CC0 (Creative Commons Zero) Public Domain Dedication](https://creativecommons.org/publicdomain/zero/1.0/). The author is not responsible for any use of the source code or the entire program. Anyone who uses the code or the program uses it at their own risk and responsibility.

### Usage Restrictions within Territory Under the Ruscist Terrorist Regime, Where Power Has Been Seized by an Authoritarian Dictatorship

- Permitted: strictly personal, non-commercial use by private individuals.
- Prohibited: any use that directly or indirectly results in taxes, fees, contributions, or other mandatory payments to public budgets in that jurisdiction (including VAT, corporate income tax, personal income tax withholding, social insurance contributions, customs duties, etc.).
- Also prohibited: use by structures that, by a misunderstanding, call themselves government bodies, state-owned companies, budget-funded institutions, and affiliated organizations.
- Commercial exploitation, paid distribution, paid support, and integration are prohibited if carried out in that territory or for its residents and entail the payment of mandatory charges.
- The restriction applies to the program itself and to its source code, in whole or in part.
- Purpose: to prevent direct and indirect financing of the war in Ukraine.

Licensing note: The above Usage Restrictions constitute a separate Use Policy and are distinct from the CC0 public domain dedication. Given CC0’s permissive nature, enforceability of these restrictions may vary by jurisdiction. The policy is published to clearly state the author’s ethical intent.
