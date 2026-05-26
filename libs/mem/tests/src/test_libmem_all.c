#include "sute.h"

/**
 * @brief Run all public libmem test suites
 *
 * @return Return status code
 */
Return test_libmem_all(void)
{
	INITTEST;

	bool first_header = true;

	HEADER("Allocation Lifecycle");
	SUTE(test_libmem_0000,"Numbered README examples for public libmem usage...");
	TEST(test_libmem_0001,"Copying an empty data descriptor clears a populated destination…");

	HEADER("Repeated Type Coverage");
	SUTE(test_libmem_0005,"Random-size bounded-buffer imports across element types (SHA-512)…");

	HEADER("Telemetry");
	SUTE(test_libmem_0009,"Telemetry counters track libmem state transitions…");

	HEADER("Typed Access");
	TEST(test_libmem_0010,"Typed point descriptors and m_resize flags…");
	SUTE(test_libmem_0011,"Typed point raw access and descriptor copying…");
	SUTE(test_libmem_0012,"Typed point m_concat_data after RELEASE_UNUSED shrink and regrow…");

	HEADER("Initial Modes");
	SUTE(test_libmem_0013,"m_create initial modes and fixed-string appends for byte and wide descriptors…");

	HEADER("Descriptor Collections");
	TEST(test_libmem_0069,"Descriptor-backed arrays support item access and foreach traversal…");

	HEADER("Low-Level Helpers");
	TEST(test_libmem_0052,"m_copy_buffer handles strings, structs, and clearing…");
	TEST(test_libmem_0053,"reset clears manually managed raw pointers…");
	TEST(test_libmem_0054,"guarded arithmetic helpers catch invalid size math…");

	HEADER("Data Invariants");
	TEST(test_libmem_0055,"Data-facing helpers reject descriptors with NULL data and non-zero length…");
	TEST(test_libmem_0056,"Data-mode resize keeps string metadata cleared…");
	TEST(test_libmem_0057,"Data-mode resize rejects stale string metadata…");
	TEST(test_libmem_0058,"Data-mode resize rejects reserves smaller than the logical payload…");

	HEADER("Data Transfers");
	TEST(test_libmem_0059,"libmem data wrappers support bytewise append and copy…");
	TEST(test_libmem_0060,"libmem data copy wrapper rejects partial destination tails…");
	TEST(test_libmem_0061,"libmem data wrappers support cross-type append and copy…");
	TEST(test_libmem_0062,"libmem data wrappers reject incompatible cross-type payloads…");

	HEADER("String Access");
	SUTE(test_libmem_0015,"String conversion, resize, raw access, and cleanup keep metadata coherent…");
	SUTE(test_libmem_0016,"String copy and concat helpers cover fixed, bounded, and descriptor sources…");
	TEST(test_libmem_0017,"String m_resize flags preserve terminators and zero-fill…");
	TEST(test_libmem_0067,"m_string returns soft read-only views for byte and wide string descriptors…");

	HEADER("Conversions");
	TEST(test_libmem_0020,"Explicit string/data mode conversions preserve logical payload…");
	TEST(test_libmem_0021,"Data-to-string conversion accepts already-string non-byte descriptors…");
	TEST(test_libmem_0022,"String-to-data conversion rejects zero-sized elements…");
	TEST(test_libmem_0023,"string_length supports multi-byte descriptors and cached values…");
	TEST(test_libmem_0024,"string_length rejects zero-sized elements…");
	TEST(test_libmem_0025,"Data-to-string conversion adds a multi-byte zero terminator when needed…");
	TEST(test_libmem_0026,"Data-to-string conversion reuses an existing multi-byte zero terminator…");

	HEADER("String Transfers");
	TEST(test_libmem_0027,"Bounded string concat supports multi-byte buffers and softly clamped self-aliasing…");
	TEST(test_libmem_0028,"Bounded string concat rejects byte counts that split elements…");
	TEST(test_libmem_0029,"String-facing helpers reject descriptors with NULL data and non-zero length…");
	TEST(test_libmem_0030,"String concat wrappers and empty bounded or fixed-string inputs support byte strings…");
	TEST(test_libmem_0031,"Public unbounded wrapper supports multi-byte strings and internal sources…");
	TEST(test_libmem_0032,"Source-mode string concat ignores non-zero sizes in unbounded mode…");
	TEST(test_libmem_0033,"Source-mode string concat rejects internal unbounded sources past the logical end…");
	TEST(test_libmem_0034,"NULL sources and empty bounded or fixed-string inputs are no-ops…");
	TEST(test_libmem_0035,"Bounded string concat still rejects internal starts past the visible terminator…");
	TEST(test_libmem_0036,"m_string_truncate shortens byte strings without resizing the descriptor…");
	TEST(test_libmem_0037,"m_string_truncate supports multi-byte strings and rejects data mode…");

	HEADER("String Invariants");
	TEST(test_libmem_0038,"m_resize rejects descriptors with an inconsistent cached string length…");
	TEST(test_libmem_0039,"m_to_string rejects stale string metadata in data mode…");
	TEST(test_libmem_0040,"m_to_string rejects inconsistent already-string metadata…");
	TEST(test_libmem_0041,"m_resize rejects string descriptors whose reserve is smaller than the logical payload…");
	TEST(test_libmem_0042,"m_string_truncate rejects descriptors whose reserve is smaller than the logical payload…");
	TEST(test_libmem_0043,"Internal replace mode supports multi-byte self-aliasing and empty aliased replacement…");
	TEST(test_libmem_0044,"Fixed-string append and m_concat_literal support byte and multi-byte descriptors…");
	TEST(test_libmem_0045,"m_concat_string dispatches to bounded and unbounded string helpers…");
	TEST(test_libmem_0046,"m_copy_string dispatches to bounded and unbounded replacement helpers…");
	TEST(test_libmem_0047,"m_copy_fixed_string and m_copy_literal replace contents through fixed-string mode…");
	TEST(test_libmem_0048,"m_finalize_string truncates through m_data() using the cached length…");
	TEST(test_libmem_0049,"m_finalize_string truncates through m_data() and adds the terminator itself…");
	TEST(test_libmem_0050,"m_finalize_string accepts an already terminated direct write…");
	TEST(test_libmem_0051,"m_finalize_string can add a terminator and rejects data descriptors…");
	TEST(test_libmem_0070,"m_formatted_string dispatches typed printf-style output for char and wchar_t string descriptors…");

	HEADER("Descriptor Boundaries");
	TEST(test_libmem_0063,"libmem string concat and raw byte concat operations…");
	TEST(test_libmem_0064,"libmem bounded raw and bounded string concat helpers…");

	HEADER("Mode Rejections");
	TEST(test_libmem_0065,"libmem mode mismatches and malformed descriptors are diagnosed…");

	HEADER("Aliasing");
	TEST(test_libmem_0066,"libmem aliasing scenarios for string and data descriptors…");

	HEADER("Global status propagation");
	TEST(test_libmem_0071,"Process-wide INFO does not skip libmem resize bookkeeping…");

	RETURN_STATUS;
}
