# Contributing to `precizer`

This guide explains how to contribute code, tests, and docs.

Looking for something to work on? Check the issue tracker: [https://github.com/precizer/precizer/issues](https://github.com/precizer/precizer/issues)
New feature requests and tasks are posted there for contributors with different levels of involvement.

## Getting Started

* Bug reports / feature requests: [https://github.com/precizer/precizer/issues/new](https://github.com/precizer/precizer/issues/new)
* Technical discussions: [https://github.com/precizer/precizer/discussions](https://github.com/precizer/precizer/discussions)
* Pull requests welcome for: code, tests, documentation, and build improvements

A few ground rules:

* One pull request per logical change.
* For anything non-trivial, align on scope/approach in an issue or discussion *before* writing a bunch of code.
* If runtime behavior changes, update tests **and** user-facing docs in the same pull request.

## AI-Assisted Development

Using AI assistants while working on the code is strongly encouraged! They catch the boring, accidental mistakes—and they reduce the typing grind so you can focus on *creation*. In the end, programming is a form of worldbuilding: designing a tiny, consistent digital reality and then making it true in code. Heh 🙂

That said, as the local demiurge of your own little system, you shouldn’t let an assistant steer the wheel or make decisions for you. Any AI-generated code must be reviewed manually—think it through, verify it, and make sure it matches your intent before it lands in the codebase.

## Local Environment

### Dependencies by Scenario

The dependency matrix below is organized by four common workflows.

Sources: `Makefile`, `tests/Makefile`, `.docker/Dockerfile.*`, `.github/workflows/precizer.yml`.
For distro-specific package details (AlmaLinux, Alpine, Arch, Debian, Gentoo, Rocky, Ubuntu), see `.docker/Dockerfile.<distro>`.

#### 1. Static Build (`make portable` or `make production`)

Required components:

* compiler: `gcc` (or `clang` when using `make clang`)
* build tool: `make`
* regex library headers: `libpcre2-dev`
* executable compressor used by the build: `upx-ucl`
* `llvm` is recommended for later sanitizer/debug workflows

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc clang make libpcre2-dev upx-ucl llvm llvm-dev
```

Note: for `portable/production`, `sqlite3` is built from `libs/sqlite3`; a system `libsqlite3-dev` package is not required for these static targets.

#### 2. Dynamic Build (`make dynamic-production`)

Additional requirements:

* system development libraries for `sqlite3` and `pcre2`

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc make libpcre2-dev libsqlite3-dev upx-ucl
```

#### 3. Test Run (`make tests`) with Sanitizers and `cmocka`

Required components:

* dependencies from sections 1 and 2
* `cmocka` (`libcmocka-dev`) for the test runner
* sanitizer toolchain (`ASan`/`UBSan`) and `llvm-symbolizer`

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

* `make portable` - statically linked portable binary (Linux)
* `make production` - static binary optimized for local CPU
* `make dynamic-production` - dynamically linked binary optimized for local CPU

Detailed build mode behavior and technical differences are documented in `README.md`, section [Building with Docker](README.md#building-with-docker).

Cleanup (recursively removes `.builds`):

```sh
make purge
```

## Code Style

* Language standard: `C2x`.
* The build uses strict warnings and `-Werror`; new code must compile cleanly.
* Match existing naming and structure in the files you touch.
* Format only what you changed:

```sh
make format
cd libs && make format
cd tests && make format
```

## Testing

Minimum before opening a pull request:

```sh
make tests
```

Where tests live:

* primary test harness: `tests/`
* test sources: `tests/src/` (naming pattern: `testXXXX.c`)
* expected output templates: `tests/templates/`
* filesystem fixtures: `tests/fixtures/`

See [TESTING](TESTING.md) for a concise overview of the testing framework: dual-path (in-process vs. black-box CLI) execution, output/state contracts, sanitizer-enabled runs, coverage reporting, and practical guidance on what to avoid when writing tests.

## Commits and Pull Requests

* Create a working branch from `main`.
* Use clear commit messages in the imperative mood.
* Don’t commit build artifacts or temporary files (`.builds/`, `precizer`, temporary `.db` files, etc.).

Include this in pull request descriptions:

1. what problem is being solved;
2. exact change scope;
3. validation commands executed (for example, `make tests`);
4. known limitations and follow-up items.

If CLI behavior changes, update `README.md` in the same pull request.

## License

By submitting changes, contributors agree that contributions are distributed under the repository licensing terms:

* `LICENSE`
* `README.md`, section `LICENSE`

