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
* If runtime behavior changes, update tests **and user-facing docs** in the same pull request.

## AI-Assisted Development

Using AI assistants while working on the code is strongly encouraged! They catch the boring, accidental mistakes—and they reduce the typing grind so you can focus on *creation*. In the end, programming is a form of worldbuilding: designing a tiny, consistent digital reality and then making it true in code. Heh 🙂

That said, as the local demiurge of your own little system, you shouldn’t let an assistant steer the wheel or make decisions for you. Any AI-generated code must be reviewed manually—think it through, verify it, and make sure it matches your intent before it lands in the codebase.

And please do not use weak AI models for programming.

## Local Environment

### Dependencies by Scenario

* Packages required to build the application are listed by operating system in the main documentation under [“Manual Build”](README.md#manual-build)
* Packages required to run an application built with dynamic system libraries are listed under [“System libraries required at runtime”](README.md#system-libraries-required-at-runtime)
* Package installation commands for supported distributions are provided in the corresponding Dockerfiles under [`.docker/`](.docker/)
* Packages required for tests are listed under [“System packages for testing”](#system-packages-for-testing)

#### Static analysis and additional tools

The `make cppcheck` target uses `bear` to create `compile_commands.json` and then runs `cppcheck`. Ubuntu and Debian require the following packages:

```sh
sudo apt-get update
sudo apt-get install -y bear cppcheck
```

Additional analysis, performance measurement, and documentation targets use Clang Static Analyzer, Valgrind, Sparse, Splint, Doxygen, Cloc, and Gource:

```sh
sudo apt-get install -y clang clang-tools valgrind sparse splint doxygen cloc gource
sudo apt-get install -y linux-tools-common linux-tools-generic linux-tools-$(uname -r)
```

The `make clang-analyzer` target automatically selects the highest `clang` version available in `PATH` and the corresponding `scan-build` version. If versioned executables are not found, the unversioned `clang` and `scan-build` commands are used.

`make spellcheck` uses `typos` from Cargo (`~/.cargo/bin/typos`):

```sh
cargo install typos-cli
```

### System packages for testing

The test suite checks individual functions, command-line application behavior, and file-processing results based on `tests/fixtures/`. SQLite and the Monocypher cryptographic library are included in the source tree. Monocypher serves as an independent reference for checking SHA512 values produced by the internal library. Separate SQLite system packages and external cryptographic packages are not required to run the tests.

The following commands install dependencies for `make tests-debug` and, except on Alpine Linux, for sanitizer-enabled `make tests`

#### Arch Linux

```sh
sudo pacman -S --needed base-devel pcre2 llvm zip unzip
```

#### Ubuntu/Debian Linux

```sh
sudo apt update
sudo apt -y install gcc make libpcre2-dev llvm libubsan1 zip unzip
```

#### Alpine Linux

```sh
sudo apk add --no-cache build-base pcre2-dev pcre2-static fts-dev argp-standalone zip unzip
```

Sanitizer mode is not supported on Alpine Linux. Tests run with `make tests-debug`.

#### Fedora Linux

```sh
sudo dnf -y install gcc make llvm libasan libubsan glibc-devel glibc-static pcre2-devel pcre2-static zip unzip
```

#### AlmaLinux 10 / Rocky Linux 10

C2x builds and tests with sanitizers use the system GCC

```sh
sudo dnf -y install dnf-plugins-core
sudo dnf config-manager --set-enabled crb
sudo dnf -y install gcc make llvm libasan libubsan glibc-devel glibc-static pcre2-devel pcre2-static zip unzip
```

#### Gentoo Linux

PCRE2 requires static library support:

```sh
echo "dev-libs/libpcre2 static-libs" | sudo tee /etc/portage/package.use/libpcre2
sudo emerge llvm-core/clang dev-libs/libpcre2 app-arch/zip app-arch/unzip
```

#### macOS

The `zip` archiver and `unzip` extraction tool are included with macOS. For the test build, install the Xcode command-line tools and Homebrew libraries:

```sh
xcode-select --install
brew install llvm pcre2 argp-standalone
```

macOS uses the dynamic sanitizer-enabled build:

```sh
make tests
```

### Clone and Build

Example of building and extracting the archive on Linux x86_64 after installing dependencies:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make production
unzip precizer.zip '*/precizer'
"./v$(make version)/precizer" --version
```

Available modes, commands, resulting executable purposes, and technical build differences are described in detail in the main documentation under [“Build variants available through Make”](README.md#build-variants-available-through-make)

Remove build files in `.builds/` while preserving completed ZIP archives in the project root:

```sh
make purge
```

## Code Style

* Language standard: `C2x`.
* The build uses strict warnings and `-Werror`; new code must compile cleanly.
* Match existing naming and structure in the files you touch.
* To format all code globally, use:

```sh
make format && (cd libs && make format) && (cd tests && make format)
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

* `COPYING`
* `README.md`, section `COPYING`
