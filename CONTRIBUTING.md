# Contributing to `precizer`

This document defines the contribution workflow for code, tests, and documentation updates.
Potential contributors can review the project Issues section at https://github.com/precizer/precizer/issues, where new feature requests are published for specialists with different levels of involvement.

## Contribution Channels

- Bug reports and feature requests: https://github.com/precizer/precizer/issues/new
- Technical discussions: https://github.com/precizer/precizer/discussions
- Pull requests: code, tests, documentation, and build improvements

## Scope and Change Hygiene

- Use one pull request per logical change.
- For non-trivial changes, align scope and approach in an issue/discussion before implementation.
- If runtime behavior changes, update tests and user-facing documentation in the same pull request.

## AI-Assisted Development

The use of AI tools for software development is explicitly encouraged. AI assistance helps reduce repetitive routine work, shifts implementation effort toward creative problem-solving, and can prevent many common low-level mistakes.

Mandatory requirement: any AI-generated or AI-modified code must be manually reviewed before submission.

## Local Environment

### Dependencies by Scenario

The dependency matrix below is organized by 4 common workflows.

Sources: `Makefile`, `tests/Makefile`, `.docker/Dockerfile.*`, `.github/workflows/precizer.yml`.
For package details on supported distributions (AlmaLinux, Alpine, Arch, Debian, Gentoo, Rocky, Ubuntu), see `.docker/Dockerfile.<distro>`.

#### 1. Static Build (`make portable` or `make production`)

Required components:

- compiler: `gcc` (or `clang` when using `make clang`)
- build tool: `make`
- regex library headers: `libpcre2-dev`
- executable compressor used by the build: `upx-ucl`
- `llvm` is recommended for later sanitizer/debug workflows

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc clang make libpcre2-dev upx-ucl llvm llvm-dev
```

Note: for `portable/production`, `sqlite3` is built from `libs/sqlite3`; a system `libsqlite3-dev` package is not required for these static targets.

#### 2. Dynamic Build (`make dynamic-production`)

Additional requirements:

- system development libraries for `sqlite3` and `pcre2`

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc make libpcre2-dev libsqlite3-dev upx-ucl
```

#### 3. Test Run (`make tests`) with Sanitizers and `cmocka`

Required components:

- dependencies from sections 1 and 2
- `cmocka` (`libcmocka-dev`) for the test runner
- sanitizer toolchain (`ASan`/`UBSan`) and `llvm-symbolizer`

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc make libpcre2-dev libsqlite3-dev llvm llvm-dev libcmocka0 libcmocka-dev upx-ucl
```

#### 4. Static Analysis and Tooling (`cppcheck` and related targets)

Minimum for `make cppcheck`:

```sh
sudo apt-get update
sudo apt-get install -y cppcheck
```

Baseline diagnostics set from `Makefile` comments:

```sh
sudo apt-get install -y cloc valgrind clang-tools cppcheck
```

Extended set for additional targets (`make analyze`, `make perf`, `make sparse-analyzer`, `make splint`, `make doc`, `make spellcheck`):

```sh
sudo apt-get install -y valgrind cppcheck clang-20 clang-tools-20 sparse splint doxygen cloc gource
sudo apt-get install -y linux-tools-common linux-tools-generic linux-tools-$(uname -r)
```

Note: `make clang-analyzer` currently uses `clang-20` and `scan-build-20` names in `Makefile`. If package names differ on the host system, adjust the environment accordingly.

`make spellcheck` uses `typos` from Cargo (`~/.cargo/bin/typos`):

```sh
cargo install typos-cli
```

### Clone and Build

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make production
./precizer --version
```

Build variants:

- `make portable` - statically linked portable binary (Linux)
- `make production` - static binary optimized for local CPU
- `make dynamic-production` - dynamically linked binary optimized for local CPU

Detailed build mode behavior and technical differences are documented in `README.md`, section [Building with Docker](README.md#building-with-docker).

Cleanup (recursively removes `.builds`):

```sh
make purge
```

## Code Style

- Language standard: `C2x`.
- The build uses strict warnings and `-Werror`; all new code must compile without warnings.
- Follow existing naming and structural patterns in modified files.
- Format only the directories affected by the change:

```sh
make format
cd libs && make format
cd tests && make format
```

## Testing

Minimum required before opening a pull request:

```sh
make tests
```

Where to add tests:

- primary test harness: `tests/`
- test sources: `tests/src/` (naming pattern: `testXXXX.c`)
- expected output templates: `tests/templates/`
- filesystem fixtures: `tests/examples/`

## Commits and Pull Requests

- Create a working branch from `main`.
- Use clear commit messages in imperative mood.
- Do not include build artifacts or temporary files (`.builds/`, `precizer`, temporary `.db` files, etc.).
- Include the following in pull request descriptions:

1. problem being solved;
2. exact change scope;
3. validation commands executed (for example, `make tests`);
4. known limitations and follow-up items.

If CLI behavior changes, update `README.md` in the same pull request.

## License

By submitting changes, contributors agree that contributions are distributed under repository licensing terms:

- `LICENSE`
- `README.md`, section `LICENSE`
