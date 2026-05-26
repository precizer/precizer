#include "test_libmem_utils.h"

/**
 * @brief Run README example 01 that intentionally shows a frame error
 *
 * This example creates a default data descriptor and then calls a string
 * helper on it. The library must reject that programmer-side frame mistake
 * with FAILURE and a clear diagnostic, while the descriptor remains safe to
 * delete afterwards
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_01_body(void)
{
	INITTEST;

	m_create(char,destinations_string);

	const char *source_string = "Hello world";

	ASSERT(FAILURE == m_copy_string(destinations_string,source_string));
	call(m_del(destinations_string));

	deliver(status);
}

/**
 * @brief Check README example 01 diagnostic capture
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_01(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0000_01[] =
	        "\\A"
	        "ERROR: src/mem_core_string\\.c:mem_core_string:\\d+ Memory management; Destination must be a string descriptor Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0000_01,
		test_libmem_0000_01_body));

	RETURN_STATUS;
}

/**
 * @brief Check README example 02 with a descriptor created as MEMORY_STRING
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_02(void)
{
	INITTEST;

	m_create(char,destinations_string,MEMORY_STRING);

	const char *source_string = "Hello world";

	ASSERT(SUCCESS == m_copy_string(destinations_string,source_string));
	ASSERT(destinations_string->is_string == true);
	ASSERT(destinations_string->string_length == strlen(source_string));
	ASSERT(destinations_string->length == strlen(source_string) + 1U);
	ASSERT(strcmp(m_text(destinations_string),source_string) == 0);

	call(m_del(destinations_string));

	RETURN_STATUS;
}

/**
 * @brief Check README example 03 with explicit conversion to string mode
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_03(void)
{
	INITTEST;

	m_create(char,destinations_string);

	const char *source_string = "Hello world";

	ASSERT(destinations_string->is_string == false);
	ASSERT(SUCCESS == m_to_string(destinations_string));
	ASSERT(destinations_string->is_string == true);
	ASSERT(SUCCESS == m_copy_string(destinations_string,source_string));
	ASSERT(destinations_string->string_length == strlen(source_string));
	ASSERT(destinations_string->length == strlen(source_string) + 1U);
	ASSERT(strcmp(m_text(destinations_string),source_string) == 0);

	call(m_del(destinations_string));

	RETURN_STATUS;
}

/**
 * @brief Check README example 04 with typed data and self-aliased append
 *
 * Builds a typed point descriptor whose initial payload fits exactly in one
 * allocation block, copies it to the destination, and then appends the
 * destination to itself. The append must grow the destination reserve while
 * preserving both copies of every original point, proving that a source view
 * inside storage being resized remains valid for the operation
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_04(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,mirror);

	/* Fill one reserve block so self-append must grow the destination storage */
	const size_t original_point_count = MEMORY_BLOCK_BYTES / sizeof(point);
	ASSERT(original_point_count > 0U);
	ASSERT(SUCCESS == m_resize(points,original_point_count));

	point *p = m_data(point,points);
	ASSERT(p != NULL);

	IF(p != NULL)
	{
		for(size_t i = 0; i < points->length; ++i)
		{
			p[i] = (point){(int)i,(int)i};
		}
	}

	/* Append mirror to itself while its source payload is owned by the buffer being enlarged */
	ASSERT(SUCCESS == m_copy(mirror,points));
	const size_t allocated_before_append = mirror->actually_allocated_bytes;
	ASSERT(allocated_before_append == MEMORY_BLOCK_BYTES);
	ASSERT(SUCCESS == m_concat_data(mirror,mirror));
	ASSERT(mirror->is_string == false);
	ASSERT(mirror->single_element_size == sizeof(point));
	ASSERT(mirror->length == original_point_count * 2U);
	ASSERT(mirror->actually_allocated_bytes > allocated_before_append);

	const point *view = m_data_ro(point,mirror);
	ASSERT(view != NULL);

	IF(view != NULL)
	{
		for(size_t i = 0; i < original_point_count; ++i)
		{
			ASSERT(view[i].x == (int)i);
			ASSERT(view[i].y == (int)i);
			ASSERT(view[i + original_point_count].x == (int)i);
			ASSERT(view[i + original_point_count].y == (int)i);
		}
	}

	call(m_del(points));
	call(m_del(mirror));

	RETURN_STATUS;
}

/**
 * @brief Check README example 05 converting data to text within existing reserve
 *
 * Builds a log prefix as raw byte data in a block that already has room for a
 * trailing zero terminator. Conversion to string mode must preserve the same
 * physical allocation while expanding the logical payload by one terminator
 * element and enabling later string concatenation
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_05(void)
{
	INITTEST;

	m_create(char,log,MEMORY_DATA);

	/* Phase one stores raw bytes without string-mode terminator semantics */
	ASSERT(SUCCESS == m_copy_buffer(log,5,"GET /"));
	ASSERT(SUCCESS == m_concat_buffer(log,4,"api "));
	ASSERT(log->is_string == false);
	ASSERT(log->length == 9U);
	ASSERT(log->data != NULL);
	ASSERT(log->actually_allocated_bytes > log->length);

	const void *const data_before_conversion = log->data;
	const size_t allocated_before_conversion = log->actually_allocated_bytes;

	/* Spare reserve lets conversion add a terminator without replacing the physical buffer */
	ASSERT(SUCCESS == m_to_string(log));
	ASSERT(log->data == data_before_conversion);
	ASSERT(log->actually_allocated_bytes == allocated_before_conversion);
	ASSERT(log->is_string == true);
	ASSERT(log->string_length == strlen("GET /api "));
	ASSERT(log->length == strlen("GET /api ") + 1U);
	ASSERT(strcmp(m_text(log),"GET /api ") == 0);

	ASSERT(SUCCESS == m_concat_literal(log,"200 OK"));
	ASSERT(log->string_length == strlen("GET /api 200 OK"));
	ASSERT(log->length == strlen("GET /api 200 OK") + 1U);
	ASSERT(strcmp(m_text(log),"GET /api 200 OK") == 0);

	call(m_del(log));

	RETURN_STATUS;
}

/**
 * @brief Check README example 06 reusing a descriptor after buffer release
 *
 * Stores a string, releases its descriptor-owned buffer with @ref m_del, and
 * verifies that the descriptor is reset while preserving string mode. A later
 * string copy through the same descriptor must allocate usable storage again
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_06(void)
{
	INITTEST;

	m_create(char,greeting,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(greeting,"alive"));
	ASSERT(strcmp(m_text(greeting),"alive") == 0);

	/* Release owned storage while retaining an initialized string descriptor */
	call(m_del(greeting));
	ASSERT(greeting->data == NULL);
	ASSERT(greeting->length == 0U);
	ASSERT(greeting->string_length == 0U);
	ASSERT(greeting->is_string == true);

	/* Reuse the cleared descriptor through the API, not any pointer to its former buffer */
	ASSERT(SUCCESS == m_copy_literal(greeting,"reborn"));
	ASSERT(greeting->is_string == true);
	ASSERT(greeting->string_length == strlen("reborn"));
	ASSERT(strcmp(m_text(greeting),"reborn") == 0);

	call(m_del(greeting));

	RETURN_STATUS;
}

/**
 * @brief Check README example 07 reusing an imported string terminator
 *
 * Copies the complete terminated byte representation of `"abc"` into a data
 * descriptor. Conversion to string mode must recognize the imported zero
 * terminator, cache the visible length, and leave the logical element count
 * unchanged
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_07(void)
{
	INITTEST;

	m_create(char,buffer);

	/* Copy raw payload together with its trailing zero element in data mode */
	ASSERT(SUCCESS == m_copy_buffer(buffer,sizeof("abc"),"abc"));
	ASSERT(buffer->is_string == false);
	ASSERT(buffer->length == sizeof("abc"));

	/* Conversion recognizes the existing terminator instead of adding an element */
	ASSERT(SUCCESS == m_to_string(buffer));
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->string_length == 3U);
	ASSERT(buffer->length == sizeof("abc"));
	ASSERT(strcmp(m_text(buffer),"abc") == 0);

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Check README example 08 literal copy expands to fixed-string semantics
 *
 * Copies a C string literal through the convenience macro and verifies that
 * it stores the same visible length and terminator-inclusive logical length
 * as an explicit fixed-string copy of the same literal
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_08(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	/* The literal wrapper must derive the complete terminated array size */
	ASSERT(SUCCESS == m_copy_literal(title,"hello"));
	ASSERT(title->string_length == strlen("hello"));
	ASSERT(title->length == sizeof("hello"));
	ASSERT(strcmp(m_text(title),"hello") == 0);

	/* Spelling out sizeof(...) explicitly must produce the same string state */
	ASSERT(SUCCESS == m_copy_fixed_string(title,sizeof("hello"),"hello"));
	ASSERT(title->string_length == strlen("hello"));
	ASSERT(title->length == sizeof("hello"));
	ASSERT(strcmp(m_text(title),"hello") == 0);

	call(m_del(title));

	RETURN_STATUS;
}

/**
 * @brief Check README example 09 for formatted string construction
 *
 * Formats an unsigned value into a file-name pattern using a byte-sized string
 * descriptor. The rendered result must remain in string mode, cache its
 * visible length, and account for one final terminator in its logical length
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_09(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	ASSERT(SUCCESS == m_formatted_string(title,"file-%u.txt",7U));
	ASSERT(title->is_string == true);
	ASSERT(title->string_length == strlen("file-7.txt"));
	ASSERT(title->length == strlen("file-7.txt") + 1U);
	ASSERT(strcmp(m_text(title),"file-7.txt") == 0);

	call(m_del(title));

	RETURN_STATUS;
}

/**
 * @brief Check README example 10 that finalizes a shortened direct write
 *
 * Starts with a complete string descriptor, overwrites only a shorter visible
 * prefix through its writable buffer, and finalizes that prefix as `"ALPHA"`.
 * Finalization must update the cached string length and terminator without
 * reducing the descriptor's existing logical length
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_10(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);

	/* Prepare a longer string whose existing storage will be rewritten in place */
	ASSERT(SUCCESS == m_copy_literal(buffer,"alphabet"));
	ASSERT(buffer->length == sizeof("alphabet"));

	char *writable = m_data(char,buffer);
	ASSERT(writable != NULL);

	/* Directly replace only the visible prefix while leaving its old tail present */
	IF(writable != NULL)
	{
		writable[0] = 'A';
		writable[1] = 'L';
		writable[2] = 'P';
		writable[3] = 'H';
		writable[4] = 'A';
	}

	/* Finalization makes only the rewritten prefix visible and leaves reserve available */
	ASSERT(SUCCESS == m_finalize_string(buffer,5));
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->string_length == 5U);
	ASSERT(buffer->length == sizeof("alphabet"));
	ASSERT(strcmp(m_text(buffer),"ALPHA") == 0);

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Check README example 11 for explicit fixed-string copying
 *
 * Copies a named fixed-size character array whose final element is guaranteed
 * to be the zero terminator. This demonstrates the fixed-string contract:
 * m_copy_fixed_string(...) accepts a trusted complete source size and produces
 * a string descriptor with the matching visible and terminator-inclusive lengths
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_11(void)
{
	INITTEST;

	m_create(char,dest,MEMORY_STRING);

	const char text[] = {'H','e','l','l','o','\0'};

	ASSERT(SUCCESS == m_copy_fixed_string(dest,sizeof(text),text));
	ASSERT(dest->is_string == true);
	ASSERT(dest->string_length == strlen("Hello"));
	ASSERT(dest->length == sizeof(text));
	ASSERT(strcmp(m_text(dest),"Hello") == 0);

	call(m_del(dest));

	RETURN_STATUS;
}

/**
 * @brief Check README example 12 for explicit fixed-string append
 *
 * Starts with an existing string and appends a named fixed-size suffix whose
 * final element is guaranteed to be the zero terminator. This demonstrates
 * that m_concat_fixed_string(...) extends the visible text while trusting the
 * supplied complete source size instead of scanning for a terminator
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_12(void)
{
	INITTEST;

	m_create(char,dest,MEMORY_STRING);

	const char suffix[] = {'-','e','n','d','\0'};

	ASSERT(SUCCESS == m_copy_literal(dest,"start"));
	ASSERT(SUCCESS == m_concat_fixed_string(dest,sizeof(suffix),suffix));
	ASSERT(dest->is_string == true);
	ASSERT(dest->string_length == strlen("start-end"));
	ASSERT(dest->length == strlen("start-end") + 1U);
	ASSERT(strcmp(m_text(dest),"start-end") == 0);

	call(m_del(dest));

	RETURN_STATUS;
}

/**
 * @brief Check README example 13 for literal append and fixed-string equivalence
 *
 * Appends the same literal first through the convenience macro and then through
 * the explicit fixed-string helper. Both paths must produce matching visible
 * text and terminator-inclusive descriptor lengths because the macro supplies
 * the complete literal size automatically
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_13(void)
{
	INITTEST;

	m_create(char,dest,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(dest,"base"));
	ASSERT(SUCCESS == m_concat_literal(dest,"-suffix"));
	ASSERT(dest->string_length == strlen("base-suffix"));
	ASSERT(dest->length == strlen("base-suffix") + 1U);
	ASSERT(strcmp(m_text(dest),"base-suffix") == 0);

	ASSERT(SUCCESS == m_copy_literal(dest,"base"));
	ASSERT(SUCCESS == m_concat_fixed_string(dest,sizeof("-suffix"),"-suffix"));
	ASSERT(dest->string_length == strlen("base-suffix"));
	ASSERT(dest->length == strlen("base-suffix") + 1U);
	ASSERT(strcmp(m_text(dest),"base-suffix") == 0);

	call(m_del(dest));

	RETURN_STATUS;
}

/**
 * @brief Run README example 14 for finalized writes and descriptor concatenation
 *
 * Creates two string descriptors with reserved writable capacity, fills their
 * visible payloads through raw pointers, and finalizes their cached lengths
 * before appending one descriptor to the other and printing the concatenated
 * result for output validation
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_14_body(void)
{
	INITTEST;

	/* Keep both directly written operands in string mode from creation */
	m_create(char,first,MEMORY_STRING);
	m_create(char,second,MEMORY_STRING);

	/* Reserve writable capacity for the two strings and their terminators */
	ASSERT(SUCCESS == m_resize(first,16));
	ASSERT(SUCCESS == m_resize(second,16));

	/* Obtain raw views for the intentionally direct writes shown in the README */
	char *first_buf = m_raw_data(first);
	char *second_buf = m_raw_data(second);
	ASSERT(first_buf != NULL);
	ASSERT(second_buf != NULL);

	int first_written = -1;
	int second_written = -1;

	IF(first_buf != NULL && second_buf != NULL)
	{
		first_written = snprintf(first_buf,first->length,"Hello");
		second_written = snprintf(second_buf,second->length," world!");
	}

	ASSERT(first_written >= 0);
	ASSERT(second_written >= 0);
	ASSERT((size_t)first_written < first->length);
	ASSERT((size_t)second_written < second->length);

	/* Publish the visible lengths before concatenation consumes their cached values */
	ASSERT(SUCCESS == m_finalize_string(first,(size_t)first_written));
	ASSERT(SUCCESS == m_finalize_string(second,(size_t)second_written));
	ASSERT(SUCCESS == m_concat_strings(first,second));

	/* Validate the concatenated descriptor before printing the documented result */
	ASSERT(strcmp(m_text(first),"Hello world!") == 0);

	/* The enclosing test matches this printed result against the README promise */
	printf("%s\n",m_text(first));

	call(m_del(first));
	call(m_del(second));

	deliver(status);
}

/**
 * @brief Check README example 14 stdout and descriptor behavior
 *
 * Captures the documented construction scenario output and verifies that the
 * finalized descriptors produce exactly the concatenated string promised by
 * the example
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_14(void)
{
	INITTEST;

	static const char expected_stdout_pattern_libmem_0000_14[] =
	        "\\A"
	        "Hello world!\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stdout_pattern_libmem_0000_14,
		NULL,
		test_libmem_0000_14_body));

	RETURN_STATUS;
}

/**
 * @brief Run README example 15 that prints a safe string view and empty length
 *
 * Builds one finalized byte string for regular read-only access and keeps a
 * second string descriptor empty. The soft m_text(...) accessor must expose
 * both the completed text and a readable empty fallback view, while
 * m_string_length(...) reports zero visible elements for the empty descriptor
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_15_body(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);
	m_create(char,scratch,MEMORY_STRING);
	size_t scratch_length = 0;

	/* Reserve writable capacity before obtaining the pointer used for direct construction */
	ASSERT(SUCCESS == m_resize(buffer,32,ZERO_NEW_MEMORY));

	char *writable = m_data(char,buffer);
	ASSERT(writable != NULL);

	int written = -1;

	IF(writable != NULL)
	{
		written = snprintf(writable,buffer->length,"Hello world!");
	}

	ASSERT(written >= 0);
	ASSERT((size_t)written < buffer->length);
	ASSERT(SUCCESS == m_finalize_string(buffer,(size_t)written));

	/* Expose the finalized byte string through the soft read-only accessor */
	const char *view = m_text(buffer);
	ASSERT(view != NULL);
	ASSERT(strcmp(view,"Hello world!") == 0);

	printf("%s\n",view);

	/* An empty initialized string still exposes an empty read-only string and zero length */
	const char *scratch_view = m_text(scratch);
	ASSERT(scratch_view != NULL);
	ASSERT(strcmp(scratch_view,"") == 0);
	printf("scratch text: \"%s\"\n",scratch_view);

	ASSERT(SUCCESS == m_string_length(scratch,&scratch_length));
	ASSERT(scratch_length == 0U);

	printf("scratch length: %zu\n",scratch_length);

	call(m_del(buffer));
	call(m_del(scratch));

	deliver(status);
}

/**
 * @brief Check README example 15 stdout with empty text and length
 *
 * Captures the printed normal view, empty fallback view, and empty-string
 * length after its in-function assertions validate both m_text(...) results
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_15(void)
{
	INITTEST;

	static const char expected_stdout_pattern_libmem_0000_15[] =
	        "\\A"
	        "Hello world!\n"
	        "scratch text: \"\"\n"
	        "scratch length: 0\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stdout_pattern_libmem_0000_15,
		NULL,
		test_libmem_0000_15_body));

	RETURN_STATUS;
}

/**
 * @brief Check README example 16 for formatted message construction
 *
 * Builds a human-readable file-size message from string and numeric format
 * arguments. The formatted helper must keep the destination in string mode,
 * cache the visible rendered length, and expose the complete resulting text
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_16(void)
{
	INITTEST;

	m_create(char,message,MEMORY_STRING);

	const char *name = "archive.tar";
	const size_t size = 4096;

	ASSERT(SUCCESS == m_formatted_string(message,"File %s: %zu bytes",name,size));
	ASSERT(message->is_string == true);
	ASSERT(message->string_length == strlen("File archive.tar: 4096 bytes"));
	ASSERT(strcmp(m_text(message),"File archive.tar: 4096 bytes") == 0);

	call(m_del(message));

	RETURN_STATUS;
}

/**
 * @brief Check README example 17 for bounded copy and bounded append
 *
 * Copies a database path and appends a suffix from two source buffers whose
 * readable ranges contain bytes after their first zero terminators. The
 * bounded string helpers must keep only each visible prefix and leave the
 * destination as one coherent terminated string
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_17(void)
{
	INITTEST;

	m_create(char,db_path,MEMORY_STRING);

	const char in_memory_db_path[] = {':','m','e','m','o','r','y',':','\0','x','x'};
	const char limited_suffix[] = {'-','n','e','w','\0','x','x'};

	/* Bounded copy stops at the first terminator and ignores readable tail bytes */
	ASSERT(SUCCESS == m_copy_string(db_path,sizeof(in_memory_db_path),in_memory_db_path));
	ASSERT(db_path->is_string == true);
	ASSERT(db_path->string_length == strlen(":memory:"));
	ASSERT(db_path->length == strlen(":memory:") + 1U);
	ASSERT(strcmp(m_text(db_path),":memory:") == 0);

	/* Bounded append applies the same visible-prefix rule to a suffix buffer */
	ASSERT(SUCCESS == m_concat_string(db_path,sizeof(limited_suffix),limited_suffix));
	ASSERT(db_path->is_string == true);
	ASSERT(db_path->string_length == strlen(":memory:-new"));
	ASSERT(db_path->length == strlen(":memory:-new") + 1U);
	ASSERT(strcmp(m_text(db_path),":memory:-new") == 0);

	call(m_del(db_path));

	RETURN_STATUS;
}

/**
 * @brief Check README example 18 for direct write followed by finalization
 *
 * Allocates room for a terminated draft string, writes it through the checked
 * writable view, and finalizes the descriptor so its cached visible length
 * agrees with the directly written payload
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_18(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	const char draft[] = "draft";

	/* Prepare string storage and write a complete terminated C string
	   through the checked writable view */
	ASSERT(SUCCESS == m_resize(title,sizeof(draft)));

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	IF(title_view != NULL)
	{
		memcpy(title_view,draft,sizeof(draft));
	}

	/* Synchronize cached string metadata after the direct buffer write */
	ASSERT(SUCCESS == m_finalize_string(title,sizeof(draft) - 1U));
	ASSERT(title->is_string == true);
	ASSERT(title->string_length == strlen("draft"));
	ASSERT(title->length == sizeof(draft));
	ASSERT(strcmp(m_text(title),"draft") == 0);

	call(m_del(title));

	RETURN_STATUS;
}

/**
 * @brief Check README example 19 for unbounded and bounded string array append
 *
 * Builds a root data descriptor that owns three inline string descriptors.
 * It covers both zero-terminated and bounded sources, confirms that each
 * child remains a char string descriptor, and deletes all owned storage
 * through the array cleanup helper
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_19(void)
{
	INITTEST;

	m_create(memory,names);

	const char source[] = {'z','e','t','a','\0','x'};

	/* Append ordinary zero-terminated strings as owned child descriptors */
	ASSERT(SUCCESS == m_string_array_append(names,char,"delta"));
	ASSERT(SUCCESS == m_string_array_append(names,char,"epsilon"));

	/* Append only the visible prefix of a bounded source with tail bytes */
	ASSERT(SUCCESS == m_string_array_append(names,char,sizeof(source),source));
	ASSERT(names->is_string == false);
	ASSERT(names->single_element_size == sizeof(memory));
	ASSERT(names->length == 3U);

	const memory *items = m_data_ro(memory,names);
	ASSERT(items != NULL);

	IF(items != NULL)
	{
		/* Each array element is an owned char string descriptor */
		ASSERT(items[0].is_string == true);
		ASSERT(items[0].single_element_size == sizeof(char));
		ASSERT(strcmp(m_text(&items[0]),"delta") == 0);
		ASSERT(items[1].is_string == true);
		ASSERT(items[1].single_element_size == sizeof(char));
		ASSERT(strcmp(m_text(&items[1]),"epsilon") == 0);
		ASSERT(items[2].is_string == true);
		ASSERT(items[2].single_element_size == sizeof(char));
		ASSERT(strcmp(m_text(&items[2]),"zeta") == 0);
	}

	/* Delete the child strings and their root descriptor in one operation */
	call(m_array_del(names));
	ASSERT(names->data == NULL);
	ASSERT(names->length == 0U);
	ASSERT(names->string_length == 0U);

	RETURN_STATUS;
}

/**
 * @brief Run README example 20 that iterates over a string descriptor array
 *
 * Appends three child strings, visits them through m_string_array_foreach(...),
 * prints them in insertion order, and counts the visited descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_20_body(void)
{
	INITTEST;

	m_create(memory,names);

	ASSERT(SUCCESS == m_string_array_append(names,char,"alpha"));
	ASSERT(SUCCESS == m_string_array_append(names,char,"beta"));
	ASSERT(SUCCESS == m_string_array_append(names,char,"gamma"));

	size_t item_count = 0;

	m_string_array_foreach(names,item)
	{
		printf("%s\n",m_text(item));
		++item_count;
	}

	ASSERT(item_count == 3U);

	call(m_array_del(names));

	deliver(status);
}

/**
 * @brief Check README example 20 stdout and string array traversal
 *
 * Captures the README example output and requires all appended strings to be
 * printed once in insertion order
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_20(void)
{
	INITTEST;

	static const char expected_stdout_pattern_libmem_0000_20[] =
	        "\\A"
	        "alpha\n"
	        "beta\n"
	        "gamma\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stdout_pattern_libmem_0000_20,
		NULL,
		test_libmem_0000_20_body));

	RETURN_STATUS;
}

/**
 * @brief Check README example 21 for resize flags
 *
 * Grows a typed point descriptor past one allocation block while requiring
 * the newly exposed payload to be zeroed, then shrinks it to a small prefix
 * with RELEASE_UNUSED and verifies that the excess reserve is returned
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_21(void)
{
	INITTEST;

	m_create(point,points);

	const size_t points_per_block = MEMORY_BLOCK_BYTES / sizeof(point);
	const size_t large_length = points_per_block + 1U;
	const size_t surviving_length = 4U;

	ASSERT(points_per_block > 0U);
	ASSERT(large_length > surviving_length);

	/* Grow across a reserve-block boundary and zero all newly exposed points */
	ASSERT((CRITICAL & m_resize(points,large_length,ZERO_NEW_MEMORY)) == 0);
	ASSERT(points->length == large_length);
	const size_t allocated_before_shrink = points->actually_allocated_bytes;
	ASSERT(allocated_before_shrink > MEMORY_BLOCK_BYTES);

	point *items = m_data(point,points);
	ASSERT(items != NULL);

	IF(items != NULL)
	{
		for(size_t i = 0; i < points->length; ++i)
		{
			ASSERT(items[i].x == 0);
			ASSERT(items[i].y == 0);
		}
	}

	/* Shrink to a small prefix and require the extra reserve block to be returned */
	ASSERT((CRITICAL & m_resize(points,surviving_length,RELEASE_UNUSED)) == 0);
	ASSERT(points->length == surviving_length);
	ASSERT(points->data != NULL);
	ASSERT(points->actually_allocated_bytes == MEMORY_BLOCK_BYTES);
	ASSERT(points->actually_allocated_bytes < allocated_before_shrink);

	call(m_del(points));

	RETURN_STATUS;
}

/**
 * @brief Verify all numbered C examples maintained in the libmem README
 *
 * This suite is a living index of README examples. Each nested test is
 * numbered so the README can link to the matching regression case directly.
 * The checks intentionally validate both the visible result and the descriptor
 * metadata that makes the example safe: mode, logical length, cached string
 * length, typed access, captured output, and cleanup behavior
 *
 * @return Return describing success or failure
 */
Return test_libmem_0000(void)
{
	INITTEST;

	TEST(test_libmem_0000_01,"README example 01: data descriptor rejects string copy...");
	TEST(test_libmem_0000_02,"README example 02: MEMORY_STRING descriptor accepts string copy...");
	TEST(test_libmem_0000_03,"README example 03: m_to_string enables later string copy...");
	TEST(test_libmem_0000_04,"README example 04: typed point self-append survives destination storage growth...");
	TEST(test_libmem_0000_05,"README example 05: data-to-string conversion reuses spare reserved storage...");
	TEST(test_libmem_0000_06,"README example 06: descriptor can be reused after m_del...");
	TEST(test_libmem_0000_07,"README example 07: copied raw bytes reuse their existing terminator as a string...");
	TEST(test_libmem_0000_08,"README example 08: literal copy derives the same fixed-string size...");
	TEST(test_libmem_0000_09,"README example 09: formatted string renders file name...");
	TEST(test_libmem_0000_10,"README example 10: direct rewrite is finalized as a shorter string...");
	TEST(test_libmem_0000_11,"README example 11: fixed-size string copy keeps one terminator...");
	TEST(test_libmem_0000_12,"README example 12: fixed-size string append extends text...");
	TEST(test_libmem_0000_13,"README example 13: literal append matches fixed-string append...");
	TEST(test_libmem_0000_14,"README example 14: raw direct writes are finalized before concat...");
	TEST(test_libmem_0000_15,"README example 15: m_text exposes text and empty views with zero length...");
	TEST(test_libmem_0000_16,"README example 16: formatted message is stored in a descriptor...");
	TEST(test_libmem_0000_17,"README example 17: bounded string copy and append ignore tail bytes...");
	TEST(test_libmem_0000_18,"README example 18: direct buffer write becomes a valid string...");
	TEST(test_libmem_0000_19,"README example 19: string array append owns nested descriptors...");
	TEST(test_libmem_0000_20,"README example 20: string array foreach visits every item...");
	TEST(test_libmem_0000_21,"README example 21: m_resize flags zero and release storage...");

	RETURN_STATUS;
}
