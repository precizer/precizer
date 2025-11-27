# libmem — Typed Memory Helper

`libmem` is a small C11 helper library that wraps dynamic memory management behind a type-aware descriptor. It centralizes overflow checks, block reuse, and string-safe helpers so client code can focus on logic instead of `malloc` bookkeeping.

## Features

- **Type tracking:** `create(T, name)` pins `sizeof(T)` inside the descriptor; access macros (`data`, `cdata`) verify the element size at runtime.
- **Safe resizing:** `resize` multiplies `element_size * length` with overflow detection, aligns allocations to 4 KB blocks, and can grow without shrinking already reserved blocks.
- **Optional resize flags:** `resize` accepts an optional `RESIZEMODES` mask; `ZERO_NEW_MEMORY` zero-fills fresh bytes, while `RELEASE_UNUSED` releases surplus capacity immediately. Combine them when both behaviors are desired.
- **Convenience operations:** `copy`, `append`, `concat_strings`, and `concat_literal` reuse the same metadata to duplicate arrays, extend buffers, or concatenate C-style strings with guaranteed null terminators.
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

## Adding to Your Project

1. Add all `src/mem_*`.c files (plus `src/mem_telemetry.c`) to your build and include `src/mem.h`.
2. Define `LIBS += mem` or link the emitted static/shared library.
3. Include `-Isrc` (already handled in this repo’s Makefile) or adjust include paths accordingly.

## License

The library follows the project-wide CC0 1.0 Universal dedication (see the top-level `LICENSE`). You may copy, modify, or redistribute the code without restriction, but no warranty is provided.
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
