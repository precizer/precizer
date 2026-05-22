#include "test_libmem_utils.h"

/**
 * @brief Run README example 01 that intentionally shows a frame error
 *
 * This example creates a default data descriptor and then calls a string
 * helper on it. The library must reject that programmer-side frame mistake
 * with FAILURE and a clear diagnostic, while the descriptor remains safe to
 * delete afterwards
 * *
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
 * *
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
 * *
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
 * @brief Check README example 04 with typed point data descriptors
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_04(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,mirror);

	/* The descriptor records sizeof(point), and checked access must return point views */
	ASSERT(SUCCESS == m_resize(points,5));

	point *p = m_data(point,points);
	ASSERT(p != NULL);

	IF(p != NULL)
	{
		for(size_t i = 0; i < points->length; ++i)
		{
			p[i] = (point){(int)i,(int)i};
		}
	}

	/* Copy and append keep the typed payload intact in data mode */
	ASSERT(SUCCESS == m_copy(mirror,points));
	ASSERT(SUCCESS == m_concat_data(mirror,points));
	ASSERT(mirror->is_string == false);
	ASSERT(mirror->single_element_size == sizeof(point));
	ASSERT(mirror->length == 10U);

	const point *view = m_data_ro(point,mirror);
	ASSERT(view != NULL);

	IF(view != NULL)
	{
		for(size_t i = 0; i < 5U; ++i)
		{
			ASSERT(view[i].x == (int)i);
			ASSERT(view[i].y == (int)i);
			ASSERT(view[i + 5U].x == (int)i);
			ASSERT(view[i + 5U].y == (int)i);
		}
	}

	call(m_del(points));
	call(m_del(mirror));

	RETURN_STATUS;
}

/**
 * @brief Check README example 05 that crosses the data-to-string boundary
 * *
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

	/* Conversion measures the existing payload, appends the terminator, and enables string helpers */
	ASSERT(SUCCESS == m_to_string(log));
	ASSERT(log->is_string == true);
	ASSERT(log->string_length == strlen("GET /api "));
	ASSERT(strcmp(m_text(log),"GET /api ") == 0);

	ASSERT(SUCCESS == m_concat_literal(log,"200 OK"));
	ASSERT(log->string_length == strlen("GET /api 200 OK"));
	ASSERT(log->length == strlen("GET /api 200 OK") + 1U);
	ASSERT(strcmp(m_text(log),"GET /api 200 OK") == 0);

	call(m_del(log));

	RETURN_STATUS;
}

/**
 * @brief Check README example 06 that reuses a descriptor after m_del
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_06(void)
{
	INITTEST;

	m_create(char,greeting,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(greeting,"alive"));
	ASSERT(strcmp(m_text(greeting),"alive") == 0);

	call(m_del(greeting));
	ASSERT(greeting->data == NULL);
	ASSERT(greeting->length == 0U);
	ASSERT(greeting->string_length == 0U);
	ASSERT(greeting->is_string == true);

	/* The deleted descriptor is still initialized and can receive new string storage */
	ASSERT(SUCCESS == m_copy_literal(greeting,"reborn"));
	ASSERT(greeting->is_string == true);
	ASSERT(greeting->string_length == strlen("reborn"));
	ASSERT(strcmp(m_text(greeting),"reborn") == 0);

	call(m_del(greeting));

	RETURN_STATUS;
}

/**
 * @brief Check README example 07 that converts copied raw bytes to string mode
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_07(void)
{
	INITTEST;

	m_create(char,buffer);

	ASSERT(SUCCESS == m_copy_buffer(buffer,sizeof("abc"),"abc"));
	ASSERT(buffer->is_string == false);
	ASSERT(buffer->length == sizeof("abc"));

	ASSERT(SUCCESS == m_to_string(buffer));
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->string_length == 3U);
	ASSERT(buffer->length == sizeof("abc"));
	ASSERT(strcmp(m_text(buffer),"abc") == 0);

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Check README example 08 for literal copy and fixed-string equivalence
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_08(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(title,"hello"));
	ASSERT(title->string_length == strlen("hello"));
	ASSERT(strcmp(m_text(title),"hello") == 0);

	ASSERT(SUCCESS == m_copy_fixed_string(title,sizeof("hello"),"hello"));
	ASSERT(title->string_length == strlen("hello"));
	ASSERT(title->length == sizeof("hello"));
	ASSERT(strcmp(m_text(title),"hello") == 0);

	call(m_del(title));

	RETURN_STATUS;
}

/**
 * @brief Check README example 09 for formatted string replacement
 * *
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
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_10(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(buffer,"alphabet"));

	char *writable = m_data(char,buffer);
	ASSERT(writable != NULL);

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
	ASSERT(strcmp(m_text(buffer),"ALPHA") == 0);

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Check README example 11 for explicit fixed-string copying
 * *
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
 * *
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
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_13(void)
{
	INITTEST;

	m_create(char,dest,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(dest,"base"));
	ASSERT(SUCCESS == m_concat_literal(dest,"-suffix"));
	ASSERT(dest->string_length == strlen("base-suffix"));
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
 * @brief Run README example 14 that prints a raw-written concatenated string
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_14_body(void)
{
	INITTEST;

	m_create(char,first,MEMORY_STRING);
	m_create(char,second,MEMORY_STRING);

	ASSERT(SUCCESS == m_resize(first,16));
	ASSERT(SUCCESS == m_resize(second,16));

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

	ASSERT(SUCCESS == m_finalize_string(first,(size_t)first_written));
	ASSERT(SUCCESS == m_finalize_string(second,(size_t)second_written));
	ASSERT(SUCCESS == m_concat_strings(first,second));

	const char safe_suffix[] = " (safe suffix)";

	ASSERT(SUCCESS == m_concat_fixed_string(first,sizeof(safe_suffix),safe_suffix));
	ASSERT(strcmp(m_text(first),"Hello world! (safe suffix)") == 0);

	printf("%s\n",m_text(first));

	call(m_del(first));
	call(m_del(second));

	deliver(status);
}

/**
 * @brief Check README example 14 stdout and descriptor behavior
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_14(void)
{
	INITTEST;

	static const char expected_stdout_pattern_libmem_0000_14[] =
		"\\A"
		"Hello world! \\(safe suffix\\)\n"
		"\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stdout_pattern_libmem_0000_14,
		NULL,
		test_libmem_0000_14_body));

	RETURN_STATUS;
}

/**
 * @brief Run README example 15 that prints a safe string view and empty length
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_15_body(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);
	m_create(char,scratch,MEMORY_STRING);
	size_t scratch_length = 0;

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

	const char *view = m_text(buffer);
	ASSERT(view != NULL);
	ASSERT(strcmp(view,"Hello world!") == 0);

	printf("%s\n",view);

	ASSERT(SUCCESS == m_string_length(scratch,&scratch_length));
	ASSERT(scratch_length == 0U);

	printf("scratch length: %zu\n",scratch_length);

	call(m_del(buffer));
	call(m_del(scratch));

	deliver(status);
}

/**
 * @brief Check README example 15 stdout and empty descriptor length
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_15(void)
{
	INITTEST;

	static const char expected_stdout_pattern_libmem_0000_15[] =
		"\\A"
		"Hello world!\n"
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
 * *
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
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_17(void)
{
	INITTEST;

	m_create(char,db_path,MEMORY_STRING);

	const char in_memory_db_path[] = ":memory:";
	const char limited_suffix[] = {'-','n','e','w','\0','x','x'};

	ASSERT(SUCCESS == m_copy_string(db_path,sizeof(in_memory_db_path),in_memory_db_path));
	ASSERT(strcmp(m_text(db_path),":memory:") == 0);

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
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_18(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	const char draft[] = "draft";

	ASSERT(SUCCESS == m_resize(title,sizeof(draft)));

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	IF(title_view != NULL)
	{
		memcpy(title_view,draft,sizeof(draft));
	}

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
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_19(void)
{
	INITTEST;

	m_create(memory,names);

	const char source[] = {'z','e','t','a','\0','x'};

	ASSERT(SUCCESS == m_string_array_append(names,char,"delta"));
	ASSERT(SUCCESS == m_string_array_append(names,char,"epsilon"));
	ASSERT(SUCCESS == m_string_array_append(names,char,sizeof(source),source));
	ASSERT(names->is_string == false);
	ASSERT(names->single_element_size == sizeof(memory));
	ASSERT(names->length == 3U);

	const memory *items = m_data_ro(memory,names);
	ASSERT(items != NULL);

	IF(items != NULL)
	{
		ASSERT(strcmp(m_text(&items[0]),"delta") == 0);
		ASSERT(strcmp(m_text(&items[1]),"epsilon") == 0);
		ASSERT(strcmp(m_text(&items[2]),"zeta") == 0);
	}

	call(m_array_del(names));
	ASSERT(names->data == NULL);
	ASSERT(names->length == 0U);
	ASSERT(names->string_length == 0U);

	RETURN_STATUS;
}

/**
 * @brief Run README example 20 that iterates over a string descriptor array
 * *
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
 * *
 * @return Return describing success or failure
 */
static Return test_libmem_0000_21(void)
{
	INITTEST;

	m_create(point,points);

	ASSERT((CRITICAL & m_resize(points,10,ZERO_NEW_MEMORY)) == 0);
	ASSERT(points->length == 10U);

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

	void *const allocated_before_shrink = points->data;

	ASSERT((CRITICAL & m_resize(points,4,RELEASE_UNUSED)) == 0);
	ASSERT(points->length == 4U);
	ASSERT(points->data != NULL || allocated_before_shrink == NULL);
	ASSERT(points->actually_allocated_bytes >= points->length * sizeof(point));

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
	TEST(test_libmem_0000_04,"README example 04: typed point descriptors copy and append data...");
	TEST(test_libmem_0000_05,"README example 05: data buffer crosses into string mode...");
	TEST(test_libmem_0000_06,"README example 06: descriptor can be reused after m_del...");
	TEST(test_libmem_0000_07,"README example 07: copied raw bytes convert to a string...");
	TEST(test_libmem_0000_08,"README example 08: literal copy matches fixed-string copy...");
	TEST(test_libmem_0000_09,"README example 09: formatted string renders file name...");
	TEST(test_libmem_0000_10,"README example 10: direct rewrite is finalized as a shorter string...");
	TEST(test_libmem_0000_11,"README example 11: fixed-size string copy keeps one terminator...");
	TEST(test_libmem_0000_12,"README example 12: fixed-size string append extends text...");
	TEST(test_libmem_0000_13,"README example 13: literal append matches fixed-string append...");
	TEST(test_libmem_0000_14,"README example 14: raw direct writes are finalized before concat...");
	TEST(test_libmem_0000_15,"README example 15: m_text and m_string_length are safe views...");
	TEST(test_libmem_0000_16,"README example 16: formatted message is stored in a descriptor...");
	TEST(test_libmem_0000_17,"README example 17: bounded string copy and append ignore tail bytes...");
	TEST(test_libmem_0000_18,"README example 18: direct buffer write becomes a valid string...");
	TEST(test_libmem_0000_19,"README example 19: string array append owns nested descriptors...");
	TEST(test_libmem_0000_20,"README example 20: string array foreach visits every item...");
	TEST(test_libmem_0000_21,"README example 21: m_resize flags zero and release storage...");

	RETURN_STATUS;
}
