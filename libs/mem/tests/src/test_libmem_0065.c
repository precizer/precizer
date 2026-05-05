#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

static const char expected_stderr_pattern[] =
	"\\A"
	"ERROR: src/mem_core_data\\.c:mem_core_data:\\d+ Memory management; Destination must be in data mode, but it is a string Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_core_data\\.c:mem_core_data:\\d+ Memory management; Destination must be in data mode, but it is a string Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_core_buffer\\.c:mem_core_buffer:\\d+ Memory management; Destination must be in data mode, but it is a string Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_core_string\\.c:mem_core_string:\\d+ Memory management; Destination must be a string descriptor Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Source and destination must both be strings or both be data Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_concat_strings\\.c:mem_concat_strings:\\d+ Memory management; Source and destination must both be strings Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Source and destination must both be strings or both be data Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_concat_strings\\.c:mem_concat_strings:\\d+ Memory management; Source and destination must both be strings Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Descriptor has non-zero length with NULL data pointer Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Descriptor has non-zero length with NULL data pointer Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Destination element size is zero \\(uninitialized\\) Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Source element size is zero \\(uninitialized\\) Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Element size mismatch \\([0-9]+ vs [0-9]+\\) Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_concat_strings\\.c:mem_concat_strings:\\d+ Memory management; Element size mismatch \\([0-9]+ vs [0-9]+\\) Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_concat_strings\\.c:mem_concat_strings:\\d+ Memory management; Destination string descriptor is inconsistent Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_copy\\.c:mem_copy:\\d+ Memory management; Source string descriptor is inconsistent Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: src/mem_concat_strings\\.c:mem_concat_strings:\\d+ Memory management; Source string descriptor is inconsistent Errno: [^\\n]+ \\(errno: [0-9]+\\)\\Z";

/**
 * @brief Capture noisy mode mismatch negative cases
 *
 * The checked functions are expected to call report() and return FAILURE.
 * function_capture() keeps those expected diagnostics out of the common test output
 *
 * @return void
 */
static void capture_libmem_mode_mismatch_negative_cases(void)
{
	INITTEST;

	static const char text_literal[] = "text";

	m_create(char,destination);
	ASSERT(SUCCESS == m_to_string(destination));
	m_create(char,source);
	m_create(char,invalid_destination);
	m_create(char,invalid_source);
	m_create(char,invalid_size_destination);
	m_create(char,invalid_size_source);
	m_create(char,string_destination,MEMORY_STRING);
	m_create(char,stale_string_destination,MEMORY_STRING);
	m_create(char,stale_string_source,MEMORY_STRING);
	m_create(unsigned short,wide_string_source,MEMORY_STRING);

	const char raw_bytes[] = {'a','b','\0'};
	const char bounded_string[] = {'x','y','\0','z'};

	ASSERT(SUCCESS == m_copy_fixed_string(destination,sizeof(text_literal),text_literal));
	ASSERT(SUCCESS == m_copy_buffer(source,sizeof(raw_bytes),raw_bytes));

	/* Reject raw descriptor concatenation when destination is a string */
	ASSERT(FAILURE == m_concat_data(destination,source));

	ASSERT(SUCCESS == m_copy_string(destination,sizeof(raw_bytes),raw_bytes));
	ASSERT(SUCCESS == m_to_string(source));
	ASSERT(SUCCESS == m_copy_fixed_string(source,sizeof(text_literal),text_literal));

	/* Reject raw descriptor concatenation when both operands are strings */
	ASSERT(FAILURE == m_concat_data(destination,source));

	ASSERT(SUCCESS == m_copy_fixed_string(destination,sizeof(text_literal),text_literal));

	/* Reject bounded raw-buffer concatenation when destination is a string */
	ASSERT(FAILURE == m_concat_buffer(destination,sizeof(raw_bytes),raw_bytes));

	ASSERT(SUCCESS == m_to_data(destination));

	/* Reject bounded string concatenation when destination is data */
	ASSERT(FAILURE == m_concat_string(destination,sizeof(bounded_string),bounded_string));

	/* Reject descriptor copy when destination is data and source is a string */
	ASSERT(FAILURE == m_copy(destination,source));

	/* Reject descriptor concat when destination is data and source is a string */
	ASSERT(FAILURE == mem_concat_strings(destination,source));

	ASSERT(SUCCESS == m_to_data(source));
	ASSERT(SUCCESS == m_to_string(destination));
	ASSERT(SUCCESS == m_copy_fixed_string(destination,sizeof(text_literal),text_literal));

	/* Reject descriptor copy when destination is a string and source is data */
	ASSERT(FAILURE == m_copy(destination,source));

	/* Reject descriptor concat when destination is a string and source is data */
	ASSERT(FAILURE == mem_concat_strings(destination,source));

	invalid_destination->length = 1;

	/* Reject descriptor copy when destination has a logical payload but NULL data */
	ASSERT(FAILURE == m_copy(invalid_destination,invalid_source));

	invalid_destination->length = 0;
	invalid_source->length = 1;

	/* Reject descriptor copy when source has a logical payload but NULL data */
	ASSERT(FAILURE == m_copy(invalid_destination,invalid_source));

	invalid_size_destination->single_element_size = 0;

	/* Reject descriptor copy when destination element size is zero */
	ASSERT(FAILURE == m_copy(invalid_size_destination,invalid_size_source));

	invalid_size_destination->single_element_size = sizeof(char);
	invalid_size_source->single_element_size = 0;

	/* Reject descriptor copy when source element size is zero */
	ASSERT(FAILURE == m_copy(invalid_size_destination,invalid_size_source));

	/* Reject string-route descriptor copy when element sizes differ */
	ASSERT(FAILURE == m_copy(string_destination,wide_string_source));

	/* Reject string-route descriptor concat when element sizes differ */
	ASSERT(FAILURE == mem_concat_strings(string_destination,wide_string_source));

	stale_string_destination->string_length = 1;

	/* Reject descriptor concat when destination string metadata is stale */
	ASSERT(FAILURE == mem_concat_strings(stale_string_destination,string_destination));

	stale_string_destination->string_length = 0;

	stale_string_source->string_length = 1;

	/* Reject string-route descriptor copy when source string metadata is stale */
	ASSERT(FAILURE == m_copy(stale_string_destination,stale_string_source));

	/* Reject descriptor concat when source string metadata is stale */
	ASSERT(FAILURE == mem_concat_strings(stale_string_destination,stale_string_source));

	invalid_destination->length = 0;
	invalid_source->length = 0;
	invalid_size_destination->single_element_size = sizeof(char);
	invalid_size_source->single_element_size = sizeof(char);

	call(m_del(destination));
	call(m_del(source));
	call(m_del(invalid_destination));
	call(m_del(invalid_source));
	call(m_del(invalid_size_destination));
	call(m_del(invalid_size_source));
	call(m_del(string_destination));
	call(m_del(stale_string_destination));
	call(m_del(stale_string_source));
	call(m_del(wide_string_source));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check mode mismatch negative cases are rejected quietly
 *
 * @return Return describing success or failure
 */
Return test_libmem_0065(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern);
	ASSERT(SUCCESS == m_to_string(pattern));
	Return capture_status = SUCCESS;
	const char *pattern_label = "inline libmem mode mismatch pattern";

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_mode_mismatch_negative_cases,
		captured_stdout,
		captured_stderr);

	if(capture_status != SUCCESS)
	{
		captured_status = capture_status;
		captured_failed_line = __LINE__;
	}

	ASSERT(SUCCESS == capture_status);
	ASSERT(captured_stdout->length == 0);
	ASSERT(SUCCESS == m_copy_fixed_string(pattern,sizeof(expected_stderr_pattern),expected_stderr_pattern));
	ASSERT(SUCCESS == match_pattern(captured_stderr,pattern,pattern_label));

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));
	call(m_del(pattern));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
