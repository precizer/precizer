# argp

Bundled [argp-standalone](https://github.com/tom42/argp-standalone) command-line parser, distributed under the [LGPL-2.1-or-later license](LICENSES/LGPL-2.1-or-later.txt)

macOS and musl-based Linux systems always use this bundled parser. On other supported systems, `production`, `portable`, `debug`, `sanitize`, and `coverage` builds use bundled argp, while `dynamic-production`, `distribution`, and `debug-dynamic` use the system implementation. The test suite follows the same choice: `tests-debug` and sanitizer tests use bundled argp; `tests-dynamic` uses system argp except on macOS and musl-based Linux systems

On Linux with glibc, system argp is included in libc. On MSYS2, `msys-build`, `windows-zip`, and `windows-exe` include bundled argp in the executable. The `libargp-devel` package is required only for MSYS2 builds using `dynamic-production`, `distribution`, `debug-dynamic`, or `tests-dynamic`. CMake is not required to build bundled argp

Commands run from `libs/argp`

Update the sources, configuration template, and licenses using Git:

```sh
make update
```

Build:

```sh
make debug
```

Libraries are created at `.builds/debug/libs/libargp.a` and `.builds/debug/libs/libargp.so`. The public header is `src/argp.h`

Generate only `.builds/debug/libs/obj/argp/config.h`, without building the library:

```sh
make .builds/debug/libs/obj/argp/config.h
```

Before compilation, `configure.c` generates `config.h` from the upstream `config.h.in`. Small compiler probes detect available headers, functions, variables, and types. The generator starts the compiler directly, without a shell. Probe programs are linked but never executed. The generated header and compiler diagnostics are kept in the build mode's `libs/obj/argp` directory

`CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS` select the target toolchain. Make passes these settings through the environment, combining the compilation flags for the selected mode in `CONFIG_FLAGS`. The same `CC` builds the generator, which runs during the build. On macOS, function checks respect the deployment target supplied through compiler flags or `MACOSX_DEPLOYMENT_TARGET`

Compiler settings accept whitespace-separated arguments, single or double quotes, and backslash escapes. Quoted paths may contain spaces. Shell variables, wildcards, and command substitutions are not expanded

The generator records an input signature in `config.h` and leaves the file unchanged when the settings, template, and generator are unchanged. Changed inputs trigger fresh probes. Adding `-B` to the generation command forces the checks to run again

The bundled sources retain their upstream formatting. `make format` formats only the local generator
