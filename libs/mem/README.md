# libmem — Typed Memory Helper

`libmem` is a small C2x helper library that wraps dynamic memory management behind a type-aware descriptor. It centralizes overflow checks, block reuse, and string-safe helpers so client code can focus on logic instead of `malloc` bookkeeping. The library transparently supports HugeTLB, Transparent Huge Pages, and allocated-memory alignment (TODO)

## Architecture

### Layout

```
.
├── .builds/             # build artifacts created by `make`; configuration subdirectories keep libraries in `libs/` and internal objects in `libs/obj/`
├── src/
│   ├── mem_*.c          # individual function implementations
│   ├── mem_telemetry.c  # telemetry helpers and global state
│   └── mem.h            # public API
├── tests/
│   ├── src/             # maintained usage examples and regression coverage
│   └── Makefile         # test build
└── Makefile
```

### Building

The Makefile follows a multi-configuration pattern:

```sh
# Debug build (O0 + debug symbols, outputs to .builds/debug)
make debug

# Production build (O3 + LTO, outputs to .builds/production)
make production     # or `make prod`

# Portable build (generic tune, outputs to .builds/portable)
make portable

# Address/UBSan build
make sanitize

# Clean generated artifacts
make clean

# Quickly remove the whole .builds/ artifact directory
make purge
```

All targets compile sources from `src/`, place objects under `.builds/<config>/libs/obj/`, and emit shared/static libraries into `.builds/<config>/libs/`.

### Adding to Your Project

`libmem` can be integrated in two ways: use a built library artifact, or embed the sources from `src/` directly into your project

The first option is to build `libmem` through its `Makefile` and link the resulting `libmem.a` or `libmem.so`. From `libs/mem/`, choose the configuration you need:

```sh
make debug
make production
make portable
make sanitize
```

When `libs/mem/Makefile` is run directly, artifacts are created under the local `libs/mem/.builds/<config>/libs/` directory: the static archive `libmem.a` and the shared library `libmem.so`. Object files remain internal artifacts under `libs/mem/.builds/<config>/libs/obj/` and should not be linked by the application directly

The application includes the public header from `libs/mem/src/mem.h` and adds that directory to the include path, for example `-I.../libs/mem/src`. At link time, point the linker at the artifact directory for the selected configuration, for example `-L.../libs/mem/.builds/production/libs -lmem`. If the static `libmem.a` archive is used, also link the library dependencies from the same configuration, primarily `librational`

When the umbrella `libs/Makefile` or the top-level project build drives the library build, `BUILDDIR` is overridden, so library artifacts are placed in the project-wide `.builds/<config>/libs/` directory at the repository root. The integration mechanism stays the same: include headers from `libs/mem/src/` and link against the selected configuration's `libmem` artifact

The second option is to copy the contents of `libs/mem/src/` into your project, include the `mem.h` header, and add all library `.c` files to your own build system. In this mode, your project decides how to compile the sources and where to place object files or final artifacts. The only requirements are that application code and all `libmem` sources can see `mem.h`, and that the compiler uses a compatible C2x mode. Because `libmem` depends on `librational`, that dependency must also be added to the project, either as a built `librational` artifact or as source files

## Library Guarantees

### Frame and Flow

`libmem` makes a sharp distinction between two kinds of input. Programmer mistakes belong to the **frame** — the structure that the programmer builds. Runtime data belongs to the **flow** — whatever the running program happens to push through that structure. The library treats them differently on purpose.

#### Frame

The frame is what the programmer creates. The programmer can accidentally use the library incorrectly, and the library guarantees that it informs the programmer about such mistakes through error messages.

#### Example 01. Frame error when a string operation is called on a data descriptor

Covered by [`test_libmem_0000_01`](tests/src/test_libmem_0000.c#L32)

For example, calling a string-mode helper on a descriptor that was declared for raw binary data is a frame error. The library reports it.

1. Create a descriptor with default settings:

```c
m_create(char,destinations_string);
```

2. Try to copy a string into the memory described by that descriptor:

```c
// Source string on the stack
const char *source_string = "Hello world";

// Attempt to copy from the source string into the destination descriptor
m_copy_string(destinations_string,source_string);

// Cleanup after use
m_del(destinations_string);
```

3. Design error. The descriptor was not declared as a string descriptor, so trying to store a string in it triggers a clear error message at the program-construction stage:

```
ERROR
```

4. Why the library reacts this way. Library functions behave differently for string descriptors and for data descriptors because of the nature of the payloads. Strings are zero-terminated, which has to be respected explicitly during operations such as concatenation or resize. Raw data must never be silently truncated or extended, and that requires its own algorithms. The library therefore provides separate paths for the two descriptor kinds. By default (as in the example above), a descriptor is created in data mode. The example missed the important step of declaring it as a string descriptor.

5. There are two ways to fix it.

#### Example 02. Descriptor created directly in string mode

Covered by [`test_libmem_0000_02`](tests/src/test_libmem_0000.c#L54)

Either declare the descriptor as a string descriptor at creation time:

```c
// No errors here — the descriptor is created with the flag that marks it as a
// string descriptor with arbitrary character width; in this example a one-byte
// signed `char`
m_create(char,destinations_string,MEMORY_STRING);
const char *source_string = "Hello world";
m_copy_string(destinations_string,source_string);
m_del(destinations_string);
```

#### Example 03. Data descriptor converted to string mode

Covered by [`test_libmem_0000_03`](tests/src/test_libmem_0000.c#L78)

Or convert an existing data descriptor into a string descriptor:

```c
// Also no errors. The descriptor is first created with the default
// MEMORY_DATA flag that marks it as a raw-byte descriptor, then promoted to a
// string descriptor, and the descriptor-kind checks inside m_copy_string()
// pass on the converted descriptor

m_create(char,destinations_string);
m_to_string(destinations_string);
const char *source_string = "Hello world";
m_copy_string(destinations_string,source_string);
m_del(destinations_string);
```

#### Flow

The flow is the runtime data that the running program places in the heap, processes, and clears on demand. Flow data is **inviolable**: it is assumed to be anything at all, and that must never, under any circumstances, lead to library errors, program crashes, or error messages emitted during the program's normal operation. In other words:

* The library guarantees error messages at the program-construction stage.
* The library guarantees the absence of any data-flow errors when the program code is stable.
* The library guarantees the inviolability of the data: it will not be corrupted by the library and will not bring the program down.

TODO: planned behavior for temporary memory shortage is to avoid aborting the program and move the operation into a throttled retry mode instead.

TODO: retries should follow an exponential-backoff algorithm with an upper delay bound. The library should first repeat the request almost immediately. If memory is still unavailable, the pause before the next attempt should grow progressively. Once the delay reaches its configured maximum, further retries should continue at that fixed interval.

TODO: this approach should prevent excessive retries during a temporary resource shortage and reduce unnecessary load on the system. As soon as memory becomes available again and the operation succeeds, the backoff state should be reset, and the retry cycle should start fresh for the next operation.

## Two Modes of Operation

A `memory` descriptor can be used in two distinct modes. The current state is stored in `is_string`, and the public helpers update that flag together with the related metadata. In normal use, callers select the mode by choosing the appropriate operations rather than by mutating descriptor fields manually. For explicit manual descriptor initialization in expressions, use `m_init(T)`, `m_init(T, MEMORY_DATA)`, or `m_init(char, MEMORY_STRING)`. For global and static initializers, use `m_init_static(T, ...)`.

A valid descriptor must not advertise `length > 0` while `data == NULL`. Public helpers that validate descriptor state reject that combination instead of repairing it implicitly.

For normal application code, prefer the `m_*` entry points. The `mem_*` names that still remain in `mem.h` should be treated as a low-level and internal-facing layer for libmem itself and for narrow advanced use cases.

### String mode

The descriptor represents a C string or an empty non-materialized string state.
Functions in this mode track `string_length` (the visible character count,
excluding the terminator). Operations that write string contents into a buffer
guarantee exactly one trailing `'\0'`

| API | Description |
|---|---|
| `m_copy_fixed_string(dest, source_size, source)` | Replace contents with a fixed-size string source that already ends with one terminator |
| `m_copy_literal(dest, "literal")` | Replace contents from a real narrow string literal without spelling out `sizeof(...)` manually |
| `m_copy_string(dest, n, buf)` | Replace contents with the visible part of a bounded string buffer |
| `m_finalize_string(dest, len[, flags])` | Finalize a direct string-buffer write and cache the visible length |
| `m_formatted_string(dest, fmt, ...)` | Replace contents with a printf-style formatted result; a `char` format targets a `char` descriptor and a `wchar_t` format targets a `wchar_t` descriptor; NULL `fmt` is rejected |
| `m_concat_fixed_string(dest, source_size, source)` | Append a fixed-size string source that already ends with one terminator |
| `m_concat_literal(dest, "literal")` | Append a real narrow string literal without spelling out `sizeof(...)` manually |
| `m_concat_strings(dest, src)` | Append one string descriptor to another using cached string lengths |
| `m_concat_string(dest, n, buf)` | Append a bounded string buffer only to a string descriptor |
| `m_concat_string(dest, buf)` | Append a string buffer up to the first zero-valued terminator element only to a string descriptor |
| `m_string_truncate(dest, len)` | Shorten the visible string to `len` elements without changing the descriptor's total `length` |
| `m_string(desc)` | Soft read-only string view for string descriptors of any element width |
| `m_text(desc)` | Thin `const char *` wrapper over `m_string(...)` for byte-sized string descriptors |
| `m_string_length(desc, &len)` | Measure the visible string length |
| `m_to_string(desc)` | Explicitly promote a data descriptor to string mode and cache the length; empty descriptors remain unallocated |

#### String descriptor contract

For a tightly packed string, `string_length` is usually one less than `length`. That is the normal case because `string_length` counts only the visible characters, while `length` also includes the terminating zero element

But `length == string_length + 1` is not a mandatory invariant for string mode. A string descriptor may have a logical size that is much larger than the visible text when code reserves room in advance for a later write or concatenation

For example, after copying the string `"abc"`, a descriptor will usually have `string_length == 3` and `length == 4`. If code then calls `m_resize(desc,64)`, the visible string stays the same, so `string_length` remains `3`, while `length` becomes `64`. That does not mean a new visible string appeared. It only means the descriptor now logically accommodates more elements and is ready for later appends without an immediate extra growth step

### Binary mode

The descriptor holds arbitrary bytes or typed elements. Functions in this mode
work with `length` (the total element count) and never inspect or inject null
terminators.

| API | Description |
|---|---|
| `m_copy(dest, src)` | Copy one descriptor into another when both descriptors already use the same mode |
| `m_copy_buffer(dest, n, buf)` | Copy exactly `n` bytes from a raw buffer into a data descriptor |
| `m_concat_data(dest, src)` | Append raw descriptor contents to the end of destination only when both descriptors are already in data mode |
| `m_concat_buffer(dest, n, buf)` | Append exactly `n` bytes from a raw buffer only to a data descriptor |
| `m_to_data(desc)` | Explicitly demote a byte-sized string descriptor to data mode and, when the terminator is the last logical element, remove it from `length` |

### Mode-agnostic operations

These work regardless of mode. Checked `m_data(...)` keeps the current string/data mode
unchanged. `m_raw_data(...)` returns an unchecked writable pointer without
changing descriptor mode or cached string metadata. On success the raw helpers
return the live `descriptor->data` pointer, not a copy; writable and read-only
raw views of the same descriptor expose the same backing storage.

| API | Description |
|---|---|
| `m_init(T, mode)` | Build a descriptor value without allocating storage; when the second argument is omitted, `MEMORY_DATA` is used |
| `m_init_static(T, mode)` | Build an initializer list for a global or static descriptor; when the second argument is omitted, `MEMORY_DATA` is used |
| `m_create(T, name)` | Declare a descriptor for element type `T` |
| `m_resize(desc, n[, flags])` / `mem_resize(desc, n, flags)` | Allocate or resize to `n` elements |
| `m_del(desc)` | Free a valid descriptor's allocation, clear the lengths, and preserve the current string/data mode. If the descriptor is inconsistent, return `FAILURE`, report the problem, and leave the descriptor unchanged |
| `m_data(T, desc)` | Checked writable pointer; keeps the current string/data mode unchanged |
| `m_data_ro(T, desc)` | Checked read-only pointer |
| `m_raw_data(desc)` | Unchecked writable pointer to the live descriptor buffer |
| `m_raw_data_ro(desc)` | Unchecked read-only pointer to the same live descriptor buffer |
| `m_guarded_byte_size(desc, n, &bytes)` | Safely convert a descriptor element count into bytes with overflow detection |
| `m_guarded_add(left, right, &sum)` | Safely add two `size_t` values with overflow detection |
| `m_guarded_subtract(left, right, &diff)` | Safely subtract one `size_t` from another with underflow detection |
| `m_string_array_append(array, element_type, source)` | Append one zero-terminated source as a new inline string descriptor without a separate size argument |
| `m_string_array_append(array, element_type, source_size, source)` | Append the visible prefix of a bounded source buffer as a new inline string descriptor |
| `m_array_del(array)` | Walk an array of inline string descriptors, delete each child descriptor, and then delete the root descriptor |
| `m_reset(&ptr)` | Free a raw pointer and set it to `NULL` |

### Arrays of string descriptors

Sometimes one root data descriptor such as `m_create(memory,names)` is the
cleanest way to own an array of nested string descriptors. In that setup
`m_string_array_append(...)` appends one new `memory` element, initializes it
as a string descriptor of the requested element type width, and copies either a
zero-terminated source or the visible prefix of a bounded source buffer into
it. `m_array_del(...)` later walks all child strings, calls `m_del(...)`
for each one, and finally clears the root descriptor

#### Example 19. Appending strings to an array of string descriptors

Covered by [`test_libmem_0000_19`](tests/src/test_libmem_0000.c#L761)

Small example without a separate size argument:

```c
m_create(memory,names);

if((TRIUMPH & m_string_array_append(names,char,"delta")) == 0) { return 1; }
if((TRIUMPH & m_string_array_append(names,char,"epsilon")) == 0) { return 1; }

memory *items = m_data(memory,names);

for(size_t index = 0; index < names->length; ++index)
{
	printf("%s\n",m_text(&items[index]));
}

if((TRIUMPH & m_array_del(names)) == 0) { return 1; }
```

The same example also covers a bounded source buffer:

```c
const char source[] = {'z','e','t','a','\0','x'};

m_create(memory,names);

if((TRIUMPH & m_string_array_append(names,char,sizeof(source),source)) == 0) { return 1; }
if((TRIUMPH & m_array_del(names)) == 0) { return 1; }
```

#### Example 20. Iterating over an array of string descriptors

Covered by [`test_libmem_0000_20`](tests/src/test_libmem_0000.c#L846)

This example appends three strings and prints them in insertion order.
`m_string_array_foreach(array,item)` exposes each nested string descriptor as
a `memory *`. Do not resize or delete the root array inside the loop because
either operation can invalidate the current pointer.

```c
m_create(memory,names);

if((TRIUMPH & m_string_array_append(names,char,"alpha")) == 0) { return 1; }
if((TRIUMPH & m_string_array_append(names,char,"beta")) == 0) { return 1; }
if((TRIUMPH & m_string_array_append(names,char,"gamma")) == 0) { return 1; }

m_string_array_foreach(names,item)
{
	printf("%s\n",m_text(item));
}

if((TRIUMPH & m_array_del(names)) == 0) { return 1; }
```

## Features

- **Safe self-aliasing as a mandatory rule:** library operations are expected to behave correctly when the source points into `destination`. If a specific helper still fails that case, it is treated as a defect or migration debt rather than as an alternative API rule.
- **Type tracking:** `m_create(T, name)` pins `sizeof(T)` inside the descriptor; checked access helpers (`m_data`, `m_data_ro`) verify the element size at runtime.
- **Explicit initialization:** `m_init(T)` and `m_init(T, MEMORY_DATA)` build an empty data descriptor without allocating storage for local declarations and assignments. `m_init(char, MEMORY_STRING)` builds an empty string descriptor without allocating storage. For global and static descriptors, use `m_init_static(T, ...)`.
- **Safe resizing:** `m_resize` and `mem_resize(..., flags)` recalculate the required byte size with overflow detection and can grow without shrinking already reserved blocks. For string descriptors, growing only extends the logical capacity — the visible string and its `string_length` stay unchanged. For example, a descriptor holding `"Hi"` has `length == 3` and `string_length == 2`; after `m_resize(&d, 10)` it reports `length == 10` and `string_length == 2`, so the visible string is still `"Hi"` with seven extra slots available for future writes. Always read `string_length` rather than `length` when you need the actual string size. Resize now also rejects descriptors whose `actually_allocated_bytes` no longer cover the current logical payload.
- **Optional m_resize flags:** `m_resize` accepts an optional `RESIZEMODES` mask, while `mem_resize(desc,n,flags)` receives the same mask explicitly. `ZERO_NEW_MEMORY` zero-fills fresh bytes, while `RELEASE_UNUSED` releases surplus capacity immediately. Combine them when both behaviors are desired.
- **Data mode keeps string metadata cleared:** when a descriptor is used as raw bytes or typed elements, `m_resize` and `mem_resize(..., flags)` expect `string_length == 0` and keep writing `0` there after a successful resize. If low-level code leaves stale string metadata in a data descriptor, resize fails instead of silently treating raw bytes as text.
- **Logical clearing without releasing reserve:** `m_resize(desc,0)` clears the logical length and resets the cached string length to zero, while preserving the current string/data mode. Without `RELEASE_UNUSED`, it keeps the already allocated storage available for reuse. `m_resize(desc,0,RELEASE_UNUSED)` also clears the logical contents but physically releases the storage as a resize operation. Use `m_del(desc)` when code is done with the current buffer regardless of its current logical length; `m_del(desc)` frees descriptor-owned storage, clears the lengths, and preserves the element type and current string/data mode. If the descriptor is already inconsistent, `m_del(desc)` returns `FAILURE`, reports the problem through `report(...)`, and does not try to repair the descriptor fields.
- **String truncation without realloc:** `m_string_truncate(desc,len)` makes the string shorter only at the logical level. It moves the terminating zero element to the new position and updates `string_length`, but it does not reduce the descriptor's total `length` and does not throw away already reserved memory. In practical terms, if the buffer currently holds `alphabet` and you request length `5`, callers will see `alpha` after the call even though the buffer itself keeps the same size. If the requested visible length is already in place, the helper is usually a no-op from the caller's point of view, but it may still rewrite the zero terminator so the string stays well-formed
- **Convenience operations:** `m_copy`, `m_copy_buffer`, `m_copy_string`, `m_copy_fixed_string`, `m_copy_literal`, `m_finalize_string`, `m_formatted_string`, `m_concat_data`, `m_concat_buffer`, `m_concat_strings`, `m_concat_string`, `m_concat_fixed_string`, `m_concat_literal`, `m_string_array_append`, and `m_array_del` reuse the same metadata to duplicate descriptors within one mode, import raw byte payloads, replace strings from bounded, unbounded, fixed-size terminated, true literal, or formatted sources, finalize direct string writes, append one string descriptor to another using cached lengths, append one descriptor-backed data-mode memory block to another, build arrays of inline string descriptors, and choose explicit bounded, unbounded, fixed-string, or literal string concatenation
- **Raw access helpers:** `m_raw_data(...)`/`m_raw_data_ro(...)` expose raw pointers when you already trust the descriptor, while checked variants remain available for safer code paths. Successful raw access returns the live descriptor buffer, so writable and read-only raw views of the same descriptor point at the same storage. They do not change descriptor mode and do not rewrite cached string metadata on their own.

### TODO: Memory Allocation And Reallocation

This section describes the low-level memory-management design for `libmem`. Application code does not choose directly between `malloc`, `mmap`, Huge Pages, or aligned blocks. The library owns that decision. For users, the main entry point remains `m_resize(...)`, while storage policy details stay hidden inside the library.

The logical descriptor size and the physically reserved memory size are different concepts. `length` stores the number of logical elements of size `single_element_size` that caller code sees. `actually_allocated_bytes` stores how many bytes the library actually reserved from the lower memory layer. That value may be larger than `length * single_element_size`, because the library may round the request up to the operating-system page size, a Huge Page size, or another selected reserve block.

The regular memory page size is detected directly when the descriptor's physical storage is allocated or reallocated, using `sysconf(_SC_PAGESIZE)` or `sysconf(_SC_PAGE_SIZE)`. This value is platform-dependent: many x86-64 Linux systems use 4 KiB pages, ARM/AArch64 systems often use 4 KiB, 16 KiB, or 64 KiB pages, Intel macOS usually uses 4 KiB pages, and Apple Silicon macOS often uses 16 KiB pages. These numbers are examples only. Code does not hard-code them. If page-size detection fails, `MEMORY_BLOCK_BYTES` is used as the fallback value.

The internal `mem_resize_calculate_allocation_bytes(...)` helper calculates the final physical reserve size. It takes the requested logical element count, one-element size, and the memory-backend configuration defined by build flags, guards all arithmetic against overflow, and returns the reserve size in bytes. After the storage resize succeeds, that calculated value is saved in `actually_allocated_bytes`.

The internal `mem_resize_storage(...)` helper allocates and resizes physical storage. It receives the `memory` descriptor itself: the helper reads the current `data` and `actually_allocated_bytes` from it, performs allocation or reallocation, and after a successful change writes back the new `data`, `actually_allocated_bytes`, and actual `storage_backend_type`. The `storage_backend_type` field records the storage kind that the library actually obtained, not only the backend requested by build flags. For ordinary heap memory, the backend uses `malloc`, `calloc`, and `realloc`. With this backend, applications can still use drop-in allocators such as mimalloc, jemalloc, TCMalloc, and similar libraries when they are linked into the program.

Full release of the buffer owned by a descriptor is performed by `m_del(...)`. Application code does not release `memory->data` directly and does not choose between `free(...)` and `munmap(...)`. The descriptor stores the actual type of the already allocated physical storage, and `m_del(...)` automatically selects the correct cleanup mechanism for that type: for heap-backed memory it indirectly uses `free(...)`, and for mmap-backed memory it uses `munmap(...)`.

When the `#define MEMORY_USE_ALIGNED_STORAGE` build flag is enabled, `mem_resize_storage(...)` allocates address-aligned blocks. `posix_memalign(...)` or `aligned_alloc(...)` are used for that backend: if build flags select POSIX-compatible code, the POSIX-compatible function is used, otherwise `aligned_alloc(...)` is used. Full release is still performed through `m_del(...)`. Neither C11 nor POSIX provides a portable API for reallocating an aligned block while preserving the requested alignment, so resizing such a block usually means allocating a new block, copying data, and freeing the old block. That can help some workloads, but it can also hurt performance when a descriptor is resized frequently: the library has to allocate a new block, copy the old data, and free the previous block more often.

When the `#define MEMORY_USE_HUGE_PAGES` build flag is enabled, the library tries to allocate memory through Huge Pages. On Linux, explicit HugeTLB pages are usually requested with `mmap(..., MAP_HUGETLB, ...)`. A separate `#define MEMORY_USE_TRANSPARENT_HUGE_PAGES` policy uses a regular memory mapping and gives the kernel a hint with `madvise(..., MADV_HUGEPAGE)`. Transparent Huge Pages do not guarantee that memory is actually backed by Huge Pages: the kernel makes the final decision.

Huge Pages depend on the CPU, operating system, kernel configuration, and available pre-reserved HugeTLB pool. A 2 MiB Huge Page size is common, x86-64 systems may also expose 1 GiB pages when the CPU and kernel support them, and ARM64/AArch64 sizes depend on the base page size and kernel configuration. `#define MEMORY_HUGE_PAGE_BYTES` specifies the minimum desired Huge Page size. If it is not set, the Huge Pages default is a reasonable 2 MiB.

`MEMORY_HUGE_PAGE_BYTES` is defined as a regular C expression in bytes, so the library can use it without additional parsing:

```c
#define MEMORY_USE_HUGE_PAGES
#define MEMORY_HUGE_PAGE_BYTES (2ULL * 1024ULL * 1024ULL) /* 2 MiB */
```

```c
#define MEMORY_USE_HUGE_PAGES
#define MEMORY_HUGE_PAGE_BYTES (1ULL * 1024ULL * 1024ULL * 1024ULL) /* 1 GiB */
```

Transparent Huge Pages do not use an explicit size:

```c
#define MEMORY_USE_TRANSPARENT_HUGE_PAGES
```

If Huge Pages are unavailable on the current platform, the HugeTLB pool is exhausted, or a specific request cannot be satisfied by the selected backend, the library automatically falls back to the next allowed policy: for example Transparent Huge Pages, aligned heap, or ordinary heap. This fallback matters because a memory optimization does not make an application fail while ordinary memory is still available.

Alignment and Huge Pages are not universal speedups. The Huge Pages mechanism can reduce page-table pressure and improve large working sets, but it can also increase memory use, worsen fragmentation, add copying during resize, and introduce kernel-side delays. Enable these modes only after profiling the concrete application on the concrete target platform.

`m_resize(...)` remains the high-level API. Its job is to validate the descriptor, store the logical size in `length`, preserve string invariants, apply flags such as `ZERO_NEW_MEMORY` and `RELEASE_UNUSED`, call `mem_resize_calculate_allocation_bytes(...)`, and then delegate the physical memory change to `mem_resize_storage(...)`. `m_resize(...)` and `m_del(...)` express different intentions. `m_resize(desc,0,RELEASE_UNUSED)` clears the logical size and returns the reserve as a resize operation. `m_del(desc)` is used when work with the current buffer is finished and the storage must be released regardless of the current logical length. Both paths leave the descriptor initialized: the element type and string/data mode are preserved. This separation lets the library grow multiple memory backends without complicating the application-facing API.

## Descriptor Fields

The `memory` struct tracks all state for a managed block:

| Field | Type | Description |
|---|---|---|
| `single_element_size` | `size_t` | Size in bytes of one array element. Set once by `m_create(T, name)` and never changed. |
| `actually_allocated_bytes` | `size_t` | Total bytes reserved by the allocator. This may exceed the logical payload because the helper aligns allocations to fixed-size blocks, but for a valid non-empty descriptor it must still cover the current payload. |
| `length` | `size_t` | Current number of elements in the allocated block. |
| `string_length` | `size_t` | Cached count of visible string elements, excluding the terminating zero element. In data mode this field is always `0`. |
| `is_string` | `bool` | Current mode flag. `true` means string mode, `false` means generic data mode. |
| `data` | `void *` | Pointer to the beginning of the allocated block, or `NULL` if none. A valid descriptor must not combine `data == NULL` with `length > 0`. |

## Usage Examples

Maintained usage examples for `libmem` live in `libs/mem/tests`. The links below point to stable `README-ID` markers in the regression tests, so each README example has a directly clickable test counterpart.

#### Example 04. Typed data descriptor and self-aliased concatenation

Covered by [`test_libmem_0000_04`](tests/src/test_libmem_0000.c#L110)

This example uses a typed data descriptor and then appends `mirror` to
itself. The executable test chooses an initial payload that occupies one
allocation block, so the self-append grows the destination storage. The
operation must preserve the source points even if an internal `realloc(...)`
moves the destination buffer

```c
#include "mem.h"

typedef struct { int x, y; } point;

int main(void)
{
	const size_t point_count = MEMORY_BLOCK_BYTES / sizeof(point);

	m_create(point,points);
	if((TRIUMPH & m_resize(points,point_count)) == 0) { return 1; }

	point *p = m_data(point,points); /* checked pointer */
	for(size_t i = 0; i < points->length; ++i) {
		p[i] = (point){(int)i,(int)i};
	}

	m_create(point,mirror);
	if((TRIUMPH & m_copy(mirror,points)) == 0) { return 1; }
	if((TRIUMPH & m_concat_data(mirror,mirror)) == 0) { return 1; }

	const point *view = m_data_ro(point,mirror);
	/* Both halves of view contain the original point array */

	m_del(points);
	m_del(mirror);
	return 0;
}
```

#### Example 05. Log record converted from data while reusing reserved storage

Covered by [`test_libmem_0000_05`](tests/src/test_libmem_0000.c#L173)

One descriptor can first collect raw bytes and then continue as a cached
string. In this scenario the short prefix already occupies an allocated block
with spare room for its terminating zero element, so `m_to_string(...)`
changes the logical representation without replacing the physical buffer or
changing its reserved size. If an existing reserve has no room for the
terminator, conversion may grow the storage instead

```c
m_create(char,log,MEMORY_DATA);

if((TRIUMPH & m_copy_buffer(log,5,"GET /")) == 0) { return 1; }
if((TRIUMPH & m_concat_buffer(log,4,"api ")) == 0) { return 1; }

/* The executable test verifies that this spare-reserve case keeps the same buffer */
if((TRIUMPH & m_to_string(log)) == 0) { return 1; }
if((TRIUMPH & m_concat_literal(log,"200 OK")) == 0) { return 1; }

printf("%s\n",m_text(log)); /* "GET /api 200 OK" */
m_del(log);
```

#### Example 06. Reusing a descriptor after `m_del`

Covered by [`test_libmem_0000_06`](tests/src/test_libmem_0000.c#L218)

`m_del(...)` releases the buffer owned by a valid descriptor and clears its
lengths, but preserves its element type and string/data mode. The descriptor
itself therefore remains initialized and can later receive new contents
through the library API. This guarantee applies to the descriptor, not to any
pointer obtained from its old buffer: such pointers are invalid after
`m_del(...)`

```c
m_create(char,greeting,MEMORY_STRING);

if((TRIUMPH & m_copy_literal(greeting,"alive")) == 0) { return 1; }
printf("%s\n",m_text(greeting)); /* "alive" */

m_del(greeting); /* The descriptor is empty and still in string mode */

/* Reuse greeting itself, never a pointer to its former buffer */
if((TRIUMPH & m_copy_literal(greeting,"reborn")) == 0) { return 1; }
printf("%s\n",m_text(greeting)); /* "reborn" */

m_del(greeting);
```

#### Example 08. Copying a string literal and an equivalent fixed-size string

Covered by [`test_libmem_0000_08`](tests/src/test_libmem_0000.c#L287)

For a string literal, `m_copy_literal(title,"hello")` derives the complete
array size `sizeof("hello")`, including its trailing terminator, and applies
fixed-string semantics. The explicit
`m_copy_fixed_string(title,sizeof("hello"),"hello")` form must leave the same
visible length and terminator-inclusive logical length

```c
/* The literal wrapper supplies the terminated array size automatically */
if((TRIUMPH & m_copy_literal(title,"hello")) == 0) { return 1; }

/* The same fixed-string operation with its size written explicitly */
if((TRIUMPH & m_copy_fixed_string(title,sizeof("hello"),"hello")) == 0) { return 1; }
```

Example 11 separately demonstrates direct fixed-string copying from a named
array whose final element is guaranteed to be the terminator

#### Example 09. Formatted string

Covered by [`test_libmem_0000_09`](tests/src/test_libmem_0000.c#L319)

The format and its arguments follow printf-style rules, while the rendered
result is stored as a valid string descriptor. Here `%u` renders `7U` into
`"file-7.txt"`; `string_length` describes the visible text and `length` also
includes its final zero terminator

```c
m_create(char,title,MEMORY_STRING);

if((TRIUMPH & m_formatted_string(title,"file-%u.txt",7U)) == 0) { return 1; }

printf("%s\n",m_text(title)); /* "file-7.txt" */
m_del(title);
```

#### Example 10. Direct write through `m_data` and string-state finalization

Covered by [`test_libmem_0000_10`](tests/src/test_libmem_0000.c#L346)

The string descriptor initially contains `"alphabet"`. Direct access replaces
only its first five characters, so the tail of the previous string is still
present immediately after the write. `m_finalize_string(buffer,5)` places a
terminator over the previous `'b'` and updates `string_length` to `5`, while
leaving `length == sizeof("alphabet")`: it shortens the visible string without
reducing the prepared descriptor length

```c
m_create(char,buffer,MEMORY_STRING);

if((TRIUMPH & m_copy_literal(buffer,"alphabet")) == 0) { return 1; }

char *writable = m_data(char,buffer);
if(writable == NULL) { return 1; }

/* Direct access replaces only the visible prefix of the previous string */
writable[0] = 'A';
writable[1] = 'L';
writable[2] = 'P';
writable[3] = 'H';
writable[4] = 'A';

/* Finalization makes "ALPHA" visible while keeping the previous length */
if((TRIUMPH & m_finalize_string(buffer,5)) == 0) { return 1; }

printf("%s\n",m_text(buffer)); /* "ALPHA" */
m_del(buffer);
```

#### Example 11. Copying a fixed-size string

Covered by [`test_libmem_0000_11`](tests/src/test_libmem_0000.c#L391)

Use this form when the source is a named array with a known complete size and
its final element is guaranteed to be the zero terminator. Unlike bounded or
unbounded string-copy helpers, `m_copy_fixed_string(...)` accepts that
terminator position by contract and does not search the source for it

```c
const char text[] = {'H','e','l','l','o','\0'};
m_copy_fixed_string(dest,sizeof(text),text);
```

#### Example 12. Appending a fixed-size string

Covered by [`test_libmem_0000_12`](tests/src/test_libmem_0000.c#L420)

Use this form when appending a named array whose complete size is known and
whose final element is guaranteed to be the zero terminator. The helper trusts
that contract and appends the visible suffix without searching it for a
terminator

```c
const char suffix[] = {'-','e','n','d','\0'};
m_concat_fixed_string(dest,sizeof(suffix),suffix);
```

#### Example 13. Appending a string literal

Covered by [`test_libmem_0000_13`](tests/src/test_libmem_0000.c#L450)

For a literal suffix, this macro is the preferred spelling of fixed-size
append: it derives the complete terminated array size automatically and avoids
manual size arithmetic

```c
m_concat_literal(dest,"-suffix");
/* Equivalent to:
   m_concat_fixed_string(dest,sizeof("-suffix"),"-suffix"); */
```

### String Concatenation Helper

#### Example 14. Concatenating string descriptors

Covered by [`test_libmem_0000_14`](tests/src/test_libmem_0000.c#L541)

This example concatenates two string descriptors through
`m_concat_strings(...)`.

Both descriptors are created in string mode before their buffers are written
directly. After each direct write, `m_finalize_string(...)` must be called to
publish the visible length in `string_length`. A data descriptor is rejected
by `m_concat_strings(...)`, but a string descriptor with a stale yet formally
valid `string_length == 0` can be accepted and produce logically incorrect
concatenation because the helper trusts the cached length

```c
#include "mem.h"
#include <stdio.h>

int main(void)
{
	m_create(char,first,MEMORY_STRING);
	m_create(char,second,MEMORY_STRING);

	/* Reserve writable capacity, including room for each zero terminator */
	if((TRIUMPH & m_resize(first,16)) == 0) { return 1; }
	if((TRIUMPH & m_resize(second,16)) == 0) { return 1; }

	/* Write directly; snprintf reports visible lengths without terminators */
	char *first_buf = m_raw_data(first);
	char *second_buf = m_raw_data(second);
	if(first_buf == NULL || second_buf == NULL) { return 1; }

	int first_written = snprintf(first_buf,first->length,"Hello");
	if(first_written < 0 || (size_t)first_written >= first->length) { return 1; }

	int second_written = snprintf(second_buf,second->length," world!");
	if(second_written < 0 || (size_t)second_written >= second->length) { return 1; }

	/* Publish the cached visible lengths before descriptor concatenation */
	if((TRIUMPH & m_finalize_string(first,(size_t)first_written)) == 0) { return 1; }
	if((TRIUMPH & m_finalize_string(second,(size_t)second_written)) == 0) { return 1; }

	if((TRIUMPH & m_concat_strings(first,second)) == 0) { return 1; }

	/* Print the result of concatenating the two finalized string descriptors */
	printf("%s\n",m_text(first)); /* Prints "Hello world!" */

	m_del(first);
	m_del(second);
	return 0;
}
```

### Safe String Access

`mem_string` (`m_string(...)`) is the wider soft read-only accessor for string
descriptors of arbitrary element width. It does not search for a terminator,
does not recompute visible length, and does not modify descriptor state. The
helper only checks the most basic string-descriptor invariants and otherwise
fully trusts the cached `string_length`. On gross contract violations it reports
the problem and returns shared zero-filled fallback storage instead of `NULL`.

`m_text(...)` is the byte-oriented thin wrapper over `m_string(...)`. It exists
for call sites that already know they are dealing with a byte-sized string
descriptor and want a `const char *` result instead of the generic
`const void *`. The wrapper adds no extra runtime checks of its own and keeps
the same soft-access behavior as `m_string(...)`.

Writable string construction is now explicit. Use `m_data(char,...)` when you need
checked writable access that keeps the current string/data mode unchanged, then call
`m_finalize_string(...)` to cache the visible length after direct writes into a
string descriptor. Use `m_raw_data(...)` when you intentionally want an
unchecked writable pointer and already trust the descriptor state.

#### Example 15. Safe string view through `m_text`

Covered by [`test_libmem_0000_15`](tests/src/test_libmem_0000.c#L625)

```c
#include "mem.h"
#include <stdio.h>

int main(void)
{
	m_create(char,buffer,MEMORY_STRING);
	m_create(char,scratch,MEMORY_STRING);
	size_t scratch_length = 0;

	if((TRIUMPH & m_resize(buffer,32,ZERO_NEW_MEMORY)) == 0) { return 1; }

	/* Obtain writable access after m_resize, which may have moved the buffer */
	char *writable = m_data(char,buffer);
	if(writable == NULL) { return 1; }

	int written = snprintf(writable,buffer->length,"Hello world!");
	if(written < 0 || (size_t)written >= buffer->length) { return 1; }

	if((TRIUMPH & m_finalize_string(buffer,(size_t)written)) == 0) { return 1; }

	/* Read-only pointer never returns NULL. */
	const char *view = m_text(buffer);
	printf("%s\n",view); /* Prints “Hello world!” */

	/* Even an empty initialized string descriptor exposes an empty read-only string */
	const char *scratch_view = m_text(scratch);
	printf("scratch text: \"%s\"\n",scratch_view); /* Prints scratch text: "" */

	if((TRIUMPH & m_string_length(scratch,&scratch_length)) == 0) { return 1; }
	printf("scratch length: %zu\n",scratch_length);

	m_del(buffer);
	m_del(scratch);
	return 0;
}
```

### Formatted Strings

`m_formatted_string(destination,format,...)` builds a new string inside an
existing string descriptor using printf-style formatting. The previous visible
contents of `destination` are fully replaced by the formatted result, and
`string_length` is updated automatically.

`m_formatted_string(...)` is a typed macro. A format string of type `char *` or
`const char *` selects `mem_formatted_string_char(...)`, which uses
`vsnprintf`; a format string of type `wchar_t *` or `const wchar_t *` selects
`mem_formatted_string_wchar(...)`, which uses `vswprintf`. The selected
function verifies that `destination` has the matching element width, so a
narrow format cannot be written into a `wchar_t` descriptor.

The format string is required: a typed NULL format pointer is rejected with
`FAILURE` instead of being treated as a successful empty operation. The format string must not point inside
`destination`'s own buffer: this operation replaces the descriptor contents and
is not currently a self-aliasing operation. Future `m_formatted_string(...)`
behavior should be brought in line with the library-wide self-aliasing rule.

#### Example 16. Formatted message

Covered by [`test_libmem_0000_16`](tests/src/test_libmem_0000.c#L653)

```c
#include "mem.h"
#include <stdio.h>

int main(void)
{
	m_create(char,message,MEMORY_STRING);

	const char *name = "archive.tar";
	const size_t size = 4096;

	if((TRIUMPH & m_formatted_string(message,"File %s: %zu bytes",name,size)) == 0) {
		m_del(message);
		return 1;
	}

	printf("%s\n",m_text(message));

	m_del(message);
	return 0;
}
```

### Known-Length String Data

When string payload length is already known, descriptors can be used as explicit byte buffers instead of relying on `strlen`-based flows.

- Allocate exact capacity with `m_resize(buffer,n)` for raw bytes or `m_resize(buffer,n + 1)` when a trailing null terminator is required.
- Write/read directly through `m_data(char,buffer)` (checked) or `m_raw_data(buffer)` (unchecked raw pointer) when exact writable byte-range access is needed
- Checked writable access through `m_data(...)` keeps the current string/data mode unchanged, so direct writes into a string descriptor can be followed by `m_finalize_string(...)`
- Writable access through `m_raw_data(...)` does not change descriptor mode and does not rewrite cached string metadata, so caller code is responsible for preserving any string invariants it relies on. The returned pointer is the live descriptor buffer and remains valid only until a resize, delete, copy, concat, or other operation that can replace or release that descriptor's storage.
- Prefer `m_data(char,buffer)` when bounded writes must fail explicitly on invalid descriptors, including `length > 0` with `data == NULL`
- Use `m_concat_data(destination,source)` to append the exact element payload stored in one data descriptor to the end of another data descriptor. The helper rejects strings, never interprets either operand as a string, and never injects terminators
- Use `m_copy_buffer(destination,n,ptr)` when a bounded external buffer should be copied into a data descriptor and exactly `n` bytes must be preserved byte for byte.
- Use `m_copy_string(destination,n,ptr)` when `destination` is already in string mode and the current string should be replaced with the visible source prefix from a known-size buffer.
- Use `m_finalize_string(destination,written_length)` after direct writes through `m_data(...)` to cache the visible length in a string descriptor. The default form dispatches with `WRITE_TERMINATOR_IF_MISSING`, which inspects the element at `written_length` and writes a zero terminator only when one is not already present. Use `m_finalize_string(destination,written_length,WRITE_TERMINATOR_ALWAYS)` when the helper should overwrite that slot with a zero terminator unconditionally. In both cases the descriptor is guaranteed to be zero-terminated on return.
- Use `m_concat_buffer(destination,n,ptr)` when `destination` is already in data mode and exactly `n` bytes must be appended without inspecting payload contents.
- Use `m_concat_string(destination,n,ptr)` when `destination` is already a string and only the visible source prefix up to the first terminator element or the end of the `n`-byte source buffer should be appended. `n` is the full source buffer size in bytes, not the number of visible characters, so a regular terminated source buffer should usually pass `strlen(s) + 1` or `sizeof(array)`. `ptr == NULL` or `n == 0` are treated as an empty append. For an internal source, `n` is only the upper bound and is softly clamped to the remaining visible suffix.
- Use `m_concat_string(destination,ptr)` when `destination` is already a string, the source is guaranteed to be terminated by a zero-valued element, and the buffer size is not known ahead of time. The function scans the source until the first zero-valued terminator element. `ptr == NULL` is treated as an empty append.
- Use `m_copy_string(destination,n,ptr)` to copy a known-size source string buffer; only the visible source prefix up to the first zero-valued terminator element within the bounded source becomes the new destination string.
- Use `m_concat_string(destination,n,ptr)` to append a known-size source string buffer to a descriptor that is already in string mode; only the visible source prefix up to the first terminator element within the `n`-byte source buffer is appended. `n` is the full source buffer size in bytes and should include the trailing zero terminator element for regular terminated buffers. For an internal source, that size remains only the upper copy bound.
- Use `m_string_length(buffer,&out)` when a string view is needed. If `buffer->is_string == true`, the helper returns the cached `buffer->string_length` immediately without rescanning the payload or performing extra integrity checks. Cache computation and cache correctness are the responsibility of the descriptor-mutating helpers that build or update string-mode descriptors, which keeps repeated length queries cheap. If `buffer->is_string == false`, the helper treats the descriptor as a bounded element buffer and searches for the first zero-valued element only within `buffer->length`. The reported length is measured in elements, not bytes. Descriptors with `length > 0` and `data == NULL` are rejected as inconsistent. The function returns via `provide(...)`, so the returned status can be overridden by `global_return_status`.
- Use `m_to_string(buffer)` when a mutable descriptor should be promoted from data mode into cached string mode. The helper measures the visible prefix once in whole elements. In data mode it treats the first zero-valued element as the terminator, where every byte in that element is zero. For non-empty descriptors it appends a terminator when the current allocation does not already contain one, while empty descriptors remain unallocated. Descriptors with `length > 0` and `data == NULL` are rejected as inconsistent. Data-mode descriptors are also expected to keep `string_length == 0`; if low-level code leaves a stale cached string length there, the conversion fails instead of guessing which string state should be trusted. Descriptors that are already in string mode are accepted as a no-op only when their cached string metadata is internally consistent
- Use `m_to_data(buffer)` when a byte-sized string descriptor should be demoted back to data mode. The current implementation supports this direction only when `single_element_size == sizeof(char)`. In string mode the zero terminator is stored in the same buffer as the text, but it remains a service element: it exists so C strings have an end marker and is not part of the useful payload. During conversion to data mode, the library therefore removes that service element from the logical length. If the terminator was the last logical element, `->length` is decreased by one. For example, a string buffer containing `"abc"` usually has `->length == 4`: three characters plus the terminating zero. After `m_to_data(buffer)`, the useful payload is a three-element data sequence `a`, `b`, `c`; the zero terminator no longer belongs to the payload

#### Example 17. Copying and appending strings from bounded buffers

Covered by [`test_libmem_0000_17`](tests/src/test_libmem_0000.c#L682)

Both sources have known complete sizes and readable bytes after their first
zero terminators. `m_copy_string(...)` copies only `":memory:"`, while
`m_concat_string(...)` appends only `"-new"`; neither `x` tail becomes part of
the resulting `":memory:-new"` string

```c
m_create(char,db_path,MEMORY_STRING);
const char in_memory_db_path[] = {':','m','e','m','o','r','y',':','\0','x','x'};

if((TRIUMPH & m_copy_string(db_path,sizeof(in_memory_db_path),in_memory_db_path)) == 0) { return 1; }

const char bounded_suffix[] = {'-','n','e','w','\0','x','x'};
if((TRIUMPH & m_concat_string(db_path,sizeof(bounded_suffix),bounded_suffix)) == 0) { return 1; }
```

#### Example 18. Direct write into a preallocated string buffer

Covered by [`test_libmem_0000_18`](tests/src/test_libmem_0000.c#L719)

This example writes directly into a preallocated string buffer. `memcpy(...)`
copies `"draft"` together with its existing terminating `'\0'`. After the
direct write, `m_finalize_string(...)` sees the terminator at the expected
boundary, caches the visible length, and returns the descriptor to a
consistent string state

```c
m_create(char,title,MEMORY_STRING);
const char draft[] = "draft";

if((TRIUMPH & m_resize(title,sizeof(draft))) == 0) { return 1; }

char *title_view = m_data(char,title);
if(title_view == NULL) { return 1; }

memcpy(title_view,draft,sizeof(draft));

if((TRIUMPH & m_finalize_string(title,sizeof(draft) - 1U)) == 0) { return 1; }
```

### String Operations Overview

- Bounded binary concat: `m_concat_buffer` appends exactly `n` bytes only to a data descriptor and never inspects payload bytes.
- Descriptor raw concat: `m_concat_data` appends one descriptor-backed memory block to another only when both descriptors are already in data mode
- Bounded string concat: `m_concat_string(destination,n,buffer)` scans source only up to the first zero-valued terminator element within the provided byte limit and preserves string termination semantics. For internal sources that point back into `destination`, the byte limit acts only as an upper bound and is softly clamped to the remaining visible suffix.
- Unbounded string concat: `m_concat_string(destination,buffer)` scans source up to the first zero-valued terminator element without a separate size argument and preserves string termination semantics.

- `m_copy_fixed_string(destination,source_size,source)`: Replaces destination with a fixed-size terminated string source and enforces a trailing `'\0'`.
- `m_copy_literal(destination,"literal")`: Convenience macro for real narrow string literals; expands to `mem_copy_fixed_string(destination,sizeof("literal"),"literal")`.
- `m_copy_buffer(destination,n,buffer)`: Copies exactly `n` bytes from a bounded source buffer into a data descriptor.
- `m_concat_data(destination,source)`: Appends all source elements as raw descriptor data to the end of destination only when both `destination` and `source` are already in data mode
- `m_concat_buffer(destination,n,buffer)`: Appends exactly `n` bytes from a raw source buffer, but only when destination is already in data mode.
- `m_concat_string(destination,n,buffer)`: Appends the visible source prefix up to the first zero-valued terminator element inside the `n`-byte source buffer, but only when destination is already in string mode. `n` is a byte count for the full source buffer, not a visible character count. `buffer == NULL` or `n == 0` mean an empty append. For internal sources, `n` is only the upper copy bound and is softly clamped to the remaining visible suffix.
- `m_concat_string(destination,buffer)`: Appends source elements up to the first zero-valued terminator element, but only when destination is already in string mode. There is no separate size argument. `buffer == NULL` means an empty append.
- `m_copy_string(destination,n,buffer)`: Replaces destination with the visible part of a bounded source string buffer and keeps exactly one trailing terminator.
- `m_finalize_string(destination,len[, flags])`: Finalizes a direct write into an already-string descriptor by caching `len` in `string_length`. The default form dispatches with `WRITE_TERMINATOR_IF_MISSING`, which writes a zero terminator at `len` only when the current element there is not already zero. `WRITE_TERMINATOR_ALWAYS` asks the helper to overwrite that slot with a zero terminator unconditionally. Either way the descriptor is always zero-terminated on return.
- `m_formatted_string(destination,format,...)`: Replaces a string descriptor with a printf-style formatted result. A `char` format selects `mem_formatted_string_char(...)` through `vsnprintf`, while a `wchar_t` format selects `mem_formatted_string_wchar(...)` through `vswprintf`. A typed NULL `format` is rejected.
- `m_concat_fixed_string(destination,source_size,source)`: Appends a fixed-size terminated string source and enforces a trailing `'\0'`.
- `m_concat_literal(destination,"literal")`: Convenience macro for real narrow string literals; expands to `mem_concat_fixed_string(destination,sizeof("literal"),"literal")`.
- `m_concat_strings(destination,source)`: Appends one string descriptor to another using cached string lengths and keeps exactly one trailing `'\0'`.
- `m_data(char,destination)`: Returns a checked writable byte pointer and fails with `NULL` on invalid descriptors or type mismatch. The current descriptor mode is left unchanged.
- `m_string(source)`: Returns a soft read-only string view for a descriptor that is already in string mode. The helper trusts cached `string_length`, reports gross contract violations, and otherwise falls back to shared zero-filled storage instead of `NULL`.
- `m_text(source)`: Returns the same soft view as `m_string(source)`, but typed as `const char *` for call sites that already know the descriptor stores a byte-sized string.
- `m_string_length(source,&len)`: Returns the cached `string_length` when `source->is_string == true`; otherwise treats the descriptor as a bounded element buffer and measures the visible prefix up to the first zero-valued element or `source->length`. The reported length is measured in elements, not bytes. Descriptors with `length > 0` and `data == NULL` are rejected as inconsistent. Return value is propagated through `provide(...)`.
- `m_to_string(destination)`: Converts a mutable descriptor from data mode into cached string mode by measuring the visible prefix once in elements. In data mode the first zero-valued element acts as the terminator, so this helper also supports arbitrary non-zero element sizes. For non-empty descriptors it ensures a terminating zero element exists, while empty descriptors remain unallocated. Descriptors with `length > 0` and `data == NULL` are rejected as inconsistent. The helper also expects `string_length == 0` while the descriptor is still in data mode; stale cached string metadata causes a clean failure instead of an implicit repair. If the descriptor is already in string mode, the call remains a no-op only when `single_element_size > 0`, empty strings keep `string_length == 0`, and non-empty strings keep `string_length < length`

#### Example 07. Converting raw bytes that already include a terminator

Covered by [`test_libmem_0000_07`](tests/src/test_libmem_0000.c#L255)

Unlike example 05, this example imports the complete array representation of
the literal `"abc"`, including its trailing zero byte, into a data descriptor.
When `m_to_string(...)` later promotes that descriptor, it recognizes the
already imported terminator: `string_length` becomes `3`, while `length`
remains `sizeof("abc")`

```c
m_create(char,buffer);

/* sizeof("abc") imports the trailing zero byte as raw data */
if((TRIUMPH & m_copy_buffer(buffer,sizeof("abc"),"abc")) == 0) { return 1; }

/* Conversion reuses the imported terminator instead of adding an element */
if((TRIUMPH & m_to_string(buffer)) == 0) { return 1; }

/* buffer->is_string == true, buffer->string_length == 3,
   and buffer->length == sizeof("abc") */
```
- `m_to_data(destination)`: Converts a mutable byte-sized descriptor from string mode into data mode and decreases `length` by one when the logical trailing zero terminator was the last element. The current implementation rejects non-byte-sized string descriptors.

### Resize Behavior Flags

`RESIZEMODES` fine-tune how `m_resize` and `mem_resize(desc,n,flags)` behave. `ZERO_NEW_MEMORY` mirrors `calloc` semantics by clearing any bytes that become newly addressable, while `RELEASE_UNUSED` lets the helper return excess capacity to the OS when you trim a buffer. Separately, `m_resize(desc,0)` clears the logical contents and resets the lengths, but without `RELEASE_UNUSED` it keeps the already allocated storage available for reuse. `m_resize(desc,0,RELEASE_UNUSED)` clears the logical contents and physically releases the storage. That path is close to `m_del(desc)` in terms of releasing memory, but it remains a resize operation: it expresses the intent to set the logical size to zero and return the reserve. In both cases, a string descriptor stays a string descriptor, and a data descriptor stays a data descriptor. Use `m_del(desc)` when code is done with the current buffer regardless of its current logical length; `m_del(desc)` frees descriptor-owned storage, clears the lengths, and preserves the element type and current string/data mode. The masks can be OR-ed together.

For string descriptors, `m_resize` only extends or reduces the logical capacity — the visible string stays intact. After growing, `length` reflects the new capacity while `string_length` still equals the original visible payload. For instance, `"Hi"` with `length == 3` and `string_length == 2` becomes `length == 10` and `string_length == 2` after `m_resize(&d, 10)`. The seven extra slots are available for future writes, but the visible string is still `"Hi"`. Always use `string_length` when you need the actual string size.

For data descriptors, the rule is simpler: `string_length` must stay `0`. A successful data resize writes that `0` again, so raw buffers keep clean non-string metadata. If custom low-level code leaves `is_string == false` but `string_length != 0`, resize stops with `FAILURE` instead of guessing what that stale string state was supposed to mean.

`m_resize` also returns `FAILURE` when the existing reserve metadata is already inconsistent on entry, for example when `actually_allocated_bytes` is smaller than `length * single_element_size` or when reserved bytes are claimed while `data == NULL`. Such descriptors are treated as corrupted, so resize no longer confirms them as successful no-ops.

```c
m_create(unsigned char,packet);

if((TRIUMPH & m_resize(packet,64)) == 0) { return 1; }

/* packet->is_string == false and packet->string_length == 0 here */
```

#### Example 21. `m_resize` behavior flags

Covered by [`test_libmem_0000_21`](tests/src/test_libmem_0000.c#L874)

Most examples in this README check `TRIUMPH`. The next two intentionally use `CRITICAL` to show the narrower hard-failure style.

To make `RELEASE_UNUSED` return actual reserved capacity, this example first
grows the array beyond one memory block. Shrinking back to four elements can
then release the additional block.

```c
const size_t points_per_block = MEMORY_BLOCK_BYTES / sizeof(point);
const size_t large_length = points_per_block + 1U;

/* Grow beyond one block and zero-fill fresh points. */
if(CRITICAL & m_resize(points,large_length,ZERO_NEW_MEMORY)) {
	return 1;
}

/* Later, shrink and return the additional block. */
if(CRITICAL & m_resize(points,4,RELEASE_UNUSED)) {
	return 1;
}
```

### String Constraints and Guarantees

- Most string helper operations require byte-sized descriptors (`single_element_size == sizeof(char)`). `m_formatted_string(...)` accepts a `char` format only for a `char` descriptor and a `wchar_t` format only for a `wchar_t` descriptor. `m_string_length(...)`, `m_to_string(...)`, and `m_concat_string(...)` also support arbitrary non-zero element sizes by treating an all-zero element as the terminator.
- The library is being migrated to a unified string model in which strings processed by the library and stored in `memory` descriptors may use arbitrary non-zero element widths. Common representations include `char`, `signed char`, `unsigned char`, `char8_t`, `wchar_t`, `char16_t`, `char32_t`, and fixed-width UTF/code-unit storage such as `uint8_t`, `uint16_t`, and `uint32_t`.
- The target guarantee of that model is safe string handling for arbitrary element widths. String algorithms are defined in whole elements rather than individual bytes, and a terminator is represented by an element whose bytes are all zero.
- Safe self-aliasing is part of that required guarantee. A source that points into `destination` is considered a valid scenario that string and buffer helpers are expected to handle correctly. Any remaining mismatch with that rule is treated as incomplete migration work and should be fixed.
- A valid descriptor must not combine `length > 0` with `data == NULL`. It also must not keep `actually_allocated_bytes > 0` while `data == NULL`, and any non-empty payload must be covered by at least `length * single_element_size` reserved bytes. Helpers that validate descriptor state reject those combinations instead of repairing them implicitly.
- A descriptor with `is_string == true` is treated as a string. A descriptor with `is_string == false` is treated as generic memory, and `m_string_length(...)` searches for the first zero-valued element only within `length` when needed.
- In string mode, `m_string_length(...)` returns the cached value from `memory->string_length` without additional integrity checks or recomputation. The cost of maintaining that cache is intentionally paid when descriptor-mutating helpers update string contents, which keeps repeated `m_string_length(...)` calls inexpensive on hot paths.
- Fixed-string helpers (`m_copy_fixed_string`, `m_concat_fixed_string`) do not rescan the source. They trust `source_size` and the guarantee that the final logical element is already the terminator.
- `m_copy_buffer(destination,n,buffer)` requires `destination->is_string == false` and `n % destination->single_element_size == 0`.
- `m_copy_buffer(destination,0,NULL)` clears destination in data mode, while `m_copy_buffer(destination,n>0,NULL)` fails.
- `m_concat_buffer(destination,n,buffer)` requires `destination->is_string == false` and `n % destination->single_element_size == 0`.
- `m_concat_buffer(destination,0,NULL)` is a no-op, while `m_concat_buffer(destination,n>0,NULL)` fails.
- If `buffer` points inside the `destination` allocation, the whole exact `n` byte range must stay inside the current logical `destination` payload; valid raw self-append scenarios are supported.
- `m_concat_string(destination,n,buffer)` requires `destination->is_string == true`, `destination->single_element_size > 0`, and `n % destination->single_element_size == 0`. `n` is the full source buffer size in bytes, so regular terminated buffers should include the trailing zero terminator element.
- `m_concat_string(destination,n,buffer)` requires `destination->is_string == true`, `destination->single_element_size > 0`, and `n % destination->single_element_size == 0` when `n > 0`. `buffer == NULL` or `n == 0` mean an empty append.
- `m_concat_string(destination,buffer)` requires `destination->is_string == true` and `destination->single_element_size > 0`. `buffer == NULL` means an empty append.
- If `buffer` points inside the `destination` allocation, `m_concat_string(destination,n,buffer)` only requires a valid start: the address must be aligned to the element size and must not go past the visible string. Starting exactly on the terminator becomes a no-op, and `n` is softly clamped to the remaining visible suffix. `m_concat_string(destination,buffer)` still requires the internal source to start within the current logical string.
- `m_copy_string(destination,n,buffer)` treats `buffer == NULL` as an empty replacement and still enforces normal string termination on the destination.
- `m_formatted_string(destination,format,...)` requires `destination->is_string == true`, a non-NULL typed format pointer, and matching element types: a `char` format requires `destination->single_element_size == sizeof(char)`, while a `wchar_t` format requires `destination->single_element_size == sizeof(wchar_t)`. The format string must not point inside the `destination` allocation.
- String helpers report failures on overflow-safe size calculations before allocation/copy.
- `m_copy_fixed_string(destination,0,NULL)` and `m_concat_fixed_string(destination,0,NULL)` treat a `NULL` source as an empty fixed-size string operation.
- `m_text(...)` and `m_string(...)` never return `NULL`; both may return shared empty fallback storage.

## License

The library follows the project-wide GNU General Public License v3.0 (GPLv3) terms. See `libs/mem/COPYING` or the top-level `COPYING` file. You may copy, modify, or redistribute the code under GPLv3 conditions, but no warranty is provided

## TODO

This section collects the library's target promises and the explicit TODO items that are mentioned above, so they do not get lost in the main guide.

### Target API Promises

- Bring safe self-aliasing to every library operation. A source that points inside `destination` should be a normal supported scenario, not a special case the user has to avoid
- Bring `m_formatted_string(...)` in line with the general self-aliasing rule. Today the format string must not point inside `destination`'s own buffer because the operation can modify or reallocate that buffer before the second formatting pass
- Finish the unified string-mode migration for arbitrary element widths. The general model already describes strings in whole elements with an all-zero terminator, but some operations still have element-width restrictions
- Extend `m_to_data(...)` beyond byte-sized string descriptors if string-to-data conversion should become fully symmetric with `m_to_string(...)`

### Duplicated TODO Items From The Text

- Implement temporary-memory-shortage handling without aborting the operation: move allocation into a throttled retry mode
- Use gradually increasing retry pauses with an upper delay bound, and reset the delay state after successful allocation
- Add transparent address alignment for allocated buffers. `MEMORY_BLOCK_BYTES` should remain the fallback minimum block size when the operating-system page size cannot be detected
- Add transparent Huge Pages allocation with address alignment and separate build or configuration flags that select ordinary heap, aligned heap, explicit Huge Pages, or Transparent Huge Pages

### What Must Be Implemented For The TODO: Memory Allocation And Reallocation Section

- Move physical-reserve calculation into `mem_resize_calculate_allocation_bytes(...)`
- Move physical storage allocation and resize into `mem_resize_storage(...)`
- Add metadata for the actual physical storage type to `memory`, so `m_del(...)` and `mem_resize_storage(...)` can choose the correct low-level cleanup mechanism
- Add a dedicated field to `struct memory` for the actual physical storage type, for example `storage_backend_type` with type `MEMORY_STORAGE_BACKEND_TYPE`. This field stores the type of the already allocated block, not the desired mode for the next allocation. The value is updated only after successful allocation or physical block replacement, remains in place until `m_del(...)`, and is used by the library to choose the correct internal cleanup mechanism. The baseline type set can look like this:

```c
typedef enum MEMORY_STORAGE_BACKEND_TYPE : unsigned int
{
	MEMORY_STORAGE_EMPTY = 0u,
	MEMORY_STORAGE_HEAP,
	MEMORY_STORAGE_ALIGNED_HEAP,
	MEMORY_STORAGE_MMAP_HUGETLB,
	MEMORY_STORAGE_MMAP_TRANSPARENT_HUGE_PAGES
} MEMORY_STORAGE_BACKEND_TYPE;
```

`MEMORY_STORAGE_EMPTY` means an empty state without an allocated physical block. `data` is `NULL`, and `actually_allocated_bytes` is `0`.

`MEMORY_STORAGE_HEAP` means an ordinary heap block obtained through the heap backend.

`MEMORY_STORAGE_ALIGNED_HEAP` means an address-aligned heap block obtained through the aligned backend.

`MEMORY_STORAGE_MMAP_HUGETLB` means an mmap-backed block allocated through explicit HugeTLB pages.

`MEMORY_STORAGE_MMAP_TRANSPARENT_HUGE_PAGES` means an mmap-backed block for which the library requested Transparent Huge Pages through a kernel hint. This value records the selected backend, but it does not promise that the kernel actually backed every page with Huge Pages.

- Add page-size detection through `sysconf(...)` with fallback to `MEMORY_BLOCK_BYTES`
- Add internal physical-storage backends: ordinary heap, aligned heap, explicit Huge Pages, and Transparent Huge Pages. The backend defines how the library obtains memory and how `m_del(...)` returns it to the system
- Implement the fallback chain between allowed backends: if the selected method is unavailable or a specific request cannot be satisfied, the library automatically tries the next allowed method
- Cover the new scheme with tests for overflow, size rounding, release, fallback, and preservation of string-mode invariants
