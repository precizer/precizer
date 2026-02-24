# libmem — Typed Memory Helper

`libmem` is a small C11 helper library that wraps dynamic memory management behind a type-aware descriptor. It centralizes overflow checks, block reuse, and string-safe helpers so client code can focus on logic instead of `malloc` bookkeeping.

## Features

- **Type tracking:** `create(T, name)` pins `sizeof(T)` inside the descriptor; access macros (`data`, `cdata`) verify the element size at runtime.
- **Safe resizing:** `resize` multiplies `element_size * length` with overflow detection, aligns allocations to 4 KB blocks, and can grow without shrinking already reserved blocks.
- **Optional resize flags:** `resize` accepts an optional `RESIZEMODES` mask; `ZERO_NEW_MEMORY` zero-fills fresh bytes, while `RELEASE_UNUSED` releases surplus capacity immediately. Combine them when both behaviors are desired.
- **Convenience operations:** `copy`, `copy_buffer`, `copy_cstring`, `append`, `concat_buffer`, `concat_strings`, `concat_cstring`, and `concat_literal` reuse the same metadata to duplicate arrays, import raw byte payloads, copy/append bounded C-string data, concatenate exact byte ranges, extend buffers, or concatenate C-style strings with guaranteed null terminators.
- **Raw access helpers:** `rawdata(...)`/`rawcdata(...)` expose raw pointers when you already trust the descriptor, while checked variants remain available for safer code paths.

## Layout

```
.
├── src/
│   ├── mem_*.c          # individual function implementations
│   ├── mem_telemetry.c  # telemetry helpers and global state
│   └── mem.h            # public API
├── example/
│   └── example.c
└── Makefile
```

## Building

The Makefile follows a multi-configuration pattern:

```sh
# Debug build (O0 + sanitizers, outputs to .builds/debug)
make debug

# Production build (O3 + LTO, outputs to .builds/production)
make production     # or `make prod`

# Portable build (generic tune, outputs to .builds/portable)
make portable

# Address/UBSan build
make sanitize

# Clean generated artifacts
make clean
```

All targets compile sources from `src/`, place objects under `.builds/<config>/obj/`, and emit shared/static libraries into `.builds/<config>/libs/`.

## Usage Example

```c
#include "mem.h"

typedef struct { int x, y; } point;

int main(void)
{
	create(point,points);
	if(resize(points,5) != SUCCESS) { return 1; }

	point *p = data(point,points); /* checked pointer */
	for(size_t i = 0; i < points->length; ++i) {
		p[i] = (point){(int)i,(int)i};
	}

	create(point,mirror);
	if(copy(mirror,points) != SUCCESS) { return 1; }
	if(append(mirror,points) != SUCCESS) { return 1; }

	const point *view = cdata(point,mirror);
	/* use view... */

	del(points);
	del(mirror);
	return 0;
}
```

### String Concatenation Helper

```c
#include "mem.h"
#include <string.h>

int main(void)
{
	create(char,first);
	create(char,second);

	/* Provision raw space (includes room for null terminator). */
	if(resize(first,16) != SUCCESS || resize(second,16) != SUCCESS) {
		return 1;
	}

	/* Write strings via rawdata() pointers. */
	char *first_buf = rawdata(first);
	char *second_buf = rawdata(second);
	if(first_buf == NULL || second_buf == NULL) { return 1; }

	strcpy(first_buf,"Hello");
	strcpy(second_buf," world!");

	if(concat_strings(first,second) != SUCCESS) {
		return 1;
	}

	if(concat_literal(first," (safe helper)") != SUCCESS) {
		return 1;
	}

	const char *result = rawcdata(first);
	if(result != NULL) {
		printf("%s\n",result); /* Prints “Hello world! (safe helper)” */
	}

	del(first);
	del(second);
	return 0;
}
```

### Safe String Access

`memory_getstring` (and the `getstring(...)` macro) guarantee that a descriptor interpreted
as text always exposes a writable, null-terminated buffer. `memory_getcstring`
(`getcstring(...)`) mirrors that behavior for read-only access, returning an empty string
when descriptors are NULL, uninitialized, or missing terminators. Both helpers remove
the need for defensive checks before calling standard library routines:

```c
#include "mem.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
	create(char,buffer);
	create(char,scratch);

	if(resize(buffer,32,ZERO_NEW_MEMORY) != SUCCESS) {
		return 1;
	}

	/* Writable view stays valid after each resize. */
	char *writable = getstring(buffer);
	strcpy(writable,"Hello");
	strncat(writable," world!",buffer->length - strlen(writable) - 1);

	/* Read-only pointer never returns NULL. */
	const char *view = getcstring(buffer);
	printf("%s\n",view); /* Prints “Hello world!” */

	/* Even uninitialized descriptors degrade to an empty string safely. */
	printf("scratch length: %zu\n",strlen(getcstring(scratch)));

	del(buffer);
	del(scratch);
	return 0;
}
```

### Known-Length String Data

When string payload length is already known, descriptors can be used as explicit byte buffers instead of relying on `strlen`-based flows.

- Allocate exact capacity with `resize(buffer,n)` for raw bytes or `resize(buffer,n + 1)` when a trailing null terminator is required.
- Write/read directly through `data(char,buffer)` (checked) or `rawdata(buffer)` (unchecked raw pointer).
- Use `copy(destination,source)` and `append(destination,source)` for fixed-length transfers. Both operations use descriptor lengths directly and do not scan for terminators.
- Use `copy_buffer(destination,ptr,n)` to copy exactly `n` bytes from a known-size external array or buffer.
- Use `concat_buffer(destination,ptr,n)` to append exactly `n` bytes from a known-size external array or buffer.
- Use `copy_cstring(destination,ptr,n)` to copy a known-size source string buffer; only the visible source prefix (up to first `'\0'` or `n` bytes) is copied and null-terminated.
- Use `concat_cstring(destination,ptr,n)` to append a known-size source string buffer; only the visible source prefix (up to first `'\0'` or `n` bytes) is appended.
- Use `string_length(buffer,&out)` only when a C-string view is needed. The scan stops at the first `'\0'` or at `buffer->length`, whichever comes first. The function returns via `provide(...)`, so the returned status can be overridden by `global_return_status`.

```c
create(char,db_path);
const char in_memory_db_path[] = ":memory:";

if(copy_cstring(db_path,in_memory_db_path,sizeof(in_memory_db_path)) != SUCCESS) {
	return 1;
}

const char bounded_suffix[] = {'-','n','e','w','\0','x','x'};
if(concat_cstring(db_path,bounded_suffix,sizeof(bounded_suffix)) != SUCCESS) {
	return 1;
}
```

### String Operations Overview

- Binary pair: `copy_buffer` and `concat_buffer` operate on exact byte counts and never inspect payload bytes.
- Bounded C-string pair: `copy_cstring` and `concat_cstring` scan source only up to the first `'\0'` (within the provided byte limit) and preserve string termination semantics.

- `copy_literal(destination,literal)`: Replaces destination with a C literal and enforces a trailing `'\0'`.
- `copy_buffer(destination,buffer,n)`: Imports exactly `n` bytes from a raw source buffer.
- `concat_buffer(destination,buffer,n)`: Appends exactly `n` bytes from a raw source buffer.
- `copy_cstring(destination,buffer,n)`: Copies visible source bytes (up to first `'\0'` or `n`) and enforces one trailing `'\0'`.
- `concat_cstring(destination,buffer,n)`: Appends visible source bytes (up to first `'\0'` or `n`) and enforces one trailing `'\0'`.
- `concat_literal(destination,literal)`: Appends a C literal and enforces a trailing `'\0'`.
- `concat_strings(destination,source)`: Concatenates two descriptor-backed strings and keeps exactly one trailing `'\0'`.
- `getstring(destination)`: Returns a writable C-string pointer and repairs missing termination when possible.
- `getcstring(source)`: Returns a read-only C-string pointer and falls back to an empty string when input is invalid.
- `string_length(source,&len)`: Measures visible string length without reading past descriptor bounds (first `'\0'` or descriptor length). Return value is propagated through `provide(...)`.

### Resize Behavior Flags

`RESIZEMODES` fine-tune how `resize` behaves. `ZERO_NEW_MEMORY` mirrors `calloc` semantics by clearing any bytes that become newly addressable, while `RELEASE_UNUSED` lets the helper return excess capacity to the OS when you trim a buffer. The masks can be OR-ed together:

```c
/* Grow and zero-fill fresh points. */
if(resize(points,10,ZERO_NEW_MEMORY) != SUCCESS) {
	return 1;
}

/* Later, shrink aggressively and keep zero-fill enabled for future growth. */
if(resize(points,4,ZERO_NEW_MEMORY | RELEASE_UNUSED) != SUCCESS) {
	return 1;
}
```

### String Constraints and Guarantees

- String helper operations require byte-sized descriptors (`element_size == sizeof(char)`).
- Literal helpers (`copy_literal`, `concat_literal`) use `strlen(...)`; embedded `'\0'` bytes are not preserved as payload.
- `copy_buffer(destination,buffer,n)` requires `n % destination->element_size == 0`.
- `copy_buffer(destination,NULL,0)` clears destination, while `copy_buffer(destination,NULL,n>0)` fails.
- `concat_buffer(destination,buffer,n)` requires `n % destination->element_size == 0`.
- `concat_buffer(destination,NULL,0)` is a no-op, while `concat_buffer(destination,NULL,n>0)` fails.
- `copy_cstring(destination,buffer,n)` fails for `buffer == NULL && n > 0`; with `n == 0`, result is an empty string.
- `concat_cstring(destination,buffer,n)` fails for `buffer == NULL && n > 0`; with `n == 0`, it behaves as appending an empty string.
- String helpers report failures on overflow-safe size calculations before allocation/copy.
- `copy_literal(destination,NULL)` and `concat_literal(destination,NULL)` treat a `NULL` literal as a no-op.
- `getstring(...)` and `getcstring(...)` never return `NULL`; both may return an internal empty-string fallback.

## Adding to Your Project

1. Add all `src/mem_*`.c files (plus `src/mem_telemetry.c`) to your build and include `src/mem.h`.
2. Define `LIBS += mem` or link the emitted static/shared library.
3. Include `-Isrc` (already handled in this repo’s Makefile) or adjust include paths accordingly.

## License

The library follows the project-wide CC0 1.0 Universal dedication (see the top-level `LICENSE`). You may copy, modify, or redistribute the code without restriction, but no warranty is provided.
