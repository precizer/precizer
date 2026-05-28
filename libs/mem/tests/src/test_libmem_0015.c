#include "test_libmem_all.h"

/**
 * @brief Verify data-to-string conversion for a manually filled char descriptor
 *
 * Starts with a data-mode char descriptor, writes `xyz` through the typed
 * writable data view, and then converts the descriptor to string mode.
 * The checks prove that conversion appends the missing terminator,
 * caches the visible string length, and keeps the resulting text
 * readable through the soft string view
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0015_1(void)
{
	INITTEST;

	m_create(char,buffer);

	/* Allocate exactly three data bytes, without room for a terminator yet */
	ASSERT(SUCCESS == m_resize(buffer,3));

	/* Write a non-terminated payload through the typed writable data view */
	char *bytes = m_data(char,buffer);
	ASSERT(bytes != NULL);

	IF(bytes != NULL)
	{
		bytes[0] = 'x';
		bytes[1] = 'y';
		bytes[2] = 'z';
	}

	/* Conversion from data mode must measure the payload and add one terminator slot */
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(SUCCESS == m_to_string(buffer));
	ASSERT(buffer->string_length == 3);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 4);
	ASSERT(0 == strcmp(m_text(buffer),"xyz"));

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify string resize shrink and zero-length resize with retained reserve
 *
 * Builds a string descriptor containing `xyz`, shrinks it to three
 * logical slots, and verifies that the visible text becomes `xy`.
 * Then the descriptor is resized to zero without RELEASE_UNUSED; the
 * already allocated backing block must stay attached, while string
 * length and logical length are cleared
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0015_2(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);

	/* Seed the string descriptor with three visible bytes plus a terminator */
	ASSERT(SUCCESS == m_concat_literal(buffer,"xyz"));
	ASSERT(buffer->string_length == 3);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 4);
	ASSERT(0 == strcmp(m_text(buffer),"xyz"));

	/* Shrinking to three total slots leaves room for only two visible bytes plus the terminator */
	ASSERT(SUCCESS == m_resize(buffer,3));
	ASSERT(buffer->string_length == 2);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 3);
	ASSERT(0 == strcmp(m_text(buffer),"xy"));

	/* Remember the attached allocation; zero resize without RELEASE_UNUSED must keep it reserved */
	void *const retained_string_data = buffer->data;
	const size_t retained_string_bytes = buffer->actually_allocated_bytes;

	ASSERT(retained_string_data != NULL);
	ASSERT(retained_string_bytes > 0);

	/* Logical zeroing clears the string while keeping the backing block available for reuse */
	ASSERT(SUCCESS == m_resize(buffer,0));
	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->actually_allocated_bytes == retained_string_bytes);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(buffer),""));

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify refill after zero resize and raw writable mutation of string storage
 *
 * Reuses a string descriptor after `m_resize(buffer,0)` without
 * RELEASE_UNUSED, then writes through m_raw_data. The test proves that
 * refill reuses the retained backing block and that raw writes expose
 * the live string storage without recalculating cached metadata
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0015_3(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);
	static const char refill[] = "hi";

	/* Create and then logically clear a string while keeping its allocation */
	ASSERT(SUCCESS == m_concat_literal(buffer,"xy"));
	void *const retained_string_data = buffer->data;
	const size_t retained_string_bytes = buffer->actually_allocated_bytes;

	ASSERT(retained_string_data != NULL);
	ASSERT(retained_string_bytes > 0);
	ASSERT(SUCCESS == m_resize(buffer,0));
	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->actually_allocated_bytes == retained_string_bytes);

	/* Refill through the string API and require the previously retained block to be reused */
	ASSERT(SUCCESS == m_concat_fixed_string(buffer,sizeof(refill),refill));
	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->actually_allocated_bytes == retained_string_bytes);
	ASSERT(buffer->string_length == 2);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 3);
	ASSERT(0 == strcmp(m_text(buffer),"hi"));

	/* Raw access exposes the live storage but intentionally leaves cached metadata unchanged */
	char *raw_bytes = (char *)m_raw_data(buffer);
	ASSERT(raw_bytes != NULL);

	IF(raw_bytes != NULL)
	{
		raw_bytes[0] = 'b';
		raw_bytes[1] = 'y';
		raw_bytes[2] = '\0';
	}

	/* The same string descriptor now reads as "by" while its cached length remains valid */
	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->string_length == 2);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 3);
	ASSERT(0 == strcmp(m_text(buffer),"by"));

	call(m_del(buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify m_del clears string descriptor storage while preserving string mode
 *
 * Deletes a non-empty string descriptor and checks the postcondition:
 * allocated storage and logical counters are cleared, while the
 * descriptor's string mode flag remains set for possible later reuse
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0015_4(void)
{
	INITTEST;

	m_create(char,buffer,MEMORY_STRING);

	/* Make deletion meaningful by giving the descriptor live string storage first */
	ASSERT(SUCCESS == m_concat_literal(buffer,"hi"));
	ASSERT(buffer->data != NULL);
	ASSERT(buffer->actually_allocated_bytes > 0);

	/* m_del frees storage and lengths, but preserves the descriptor mode */
	call(m_del(buffer));
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->data == NULL);
	ASSERT(buffer->actually_allocated_bytes == 0);

	RETURN_STATUS;
}

/**
 * @brief Verify string conversion, resizing, raw access, and cleanup metadata
 *
 * In plain terms, this suite follows the life of a small char string
 * descriptor through the operations that are easiest to desynchronize:
 * converting raw bytes into a string, shrinking a string while keeping
 * its cached length correct, resizing to zero without releasing the
 * reserved block, refilling the retained block, mutating the live
 * storage through m_raw_data, and finally deleting the descriptor.
 * The nested tests keep those contracts separate so a failure points at
 * the specific state transition that broke cached string metadata
 *
 * @return Return describing success or failure
 */
Return test_libmem_0015(void)
{
	INITTEST;

	TEST(test_libmem_0015_1,"m_to_string finalizes manually filled char data");
	TEST(test_libmem_0015_2,"String resize shrink and zero resize keep metadata coherent");
	TEST(test_libmem_0015_3,"Refill after zero resize and raw mutation reuse live storage");
	TEST(test_libmem_0015_4,"m_del clears string storage while preserving string mode");

	RETURN_STATUS;
}
