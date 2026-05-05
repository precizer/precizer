#include "sute.h"

/**
 * @brief Run string example scenarios for libmem
 *
 * @return Return describing success or failure
 */
Return test_libmem_string_mode(void)
{
	INITTEST;
	bool first_header = true;

	HEADER("String Access");
	TEST(test_libmem_0015,"String resize and raw writable access keep cached metadata coherent…");
	TEST(test_libmem_0016,"String copy and concat helpers from the example…");
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
	TEST(test_libmem_0036,"mem_string_truncate shortens byte strings without resizing the descriptor…");
	TEST(test_libmem_0037,"mem_string_truncate supports multi-byte strings and rejects data mode…");

	HEADER("String Invariants");
	TEST(test_libmem_0038,"m_resize rejects descriptors with an inconsistent cached string length…");
	TEST(test_libmem_0039,"m_to_string rejects stale string metadata in data mode…");
	TEST(test_libmem_0040,"m_to_string rejects inconsistent already-string metadata…");
	TEST(test_libmem_0041,"m_resize rejects string descriptors whose reserve is smaller than the logical payload…");
	TEST(test_libmem_0042,"mem_string_truncate rejects descriptors whose reserve is smaller than the logical payload…");
	TEST(test_libmem_0043,"Internal replace mode supports multi-byte self-aliasing and empty aliased replacement…");
	TEST(test_libmem_0044,"Fixed-string append and m_concat_literal support byte and multi-byte descriptors…");
	TEST(test_libmem_0045,"m_concat_string dispatches to bounded and unbounded string helpers…");
	TEST(test_libmem_0046,"m_copy_string dispatches to bounded and unbounded replacement helpers…");
	TEST(test_libmem_0047,"m_copy_fixed_string and m_copy_literal replace contents through fixed-string mode…");
	TEST(test_libmem_0048,"m_finalize_string truncates through m_data() using the cached length…");
	TEST(test_libmem_0049,"m_finalize_string truncates through m_data() and adds the terminator itself…");
	TEST(test_libmem_0050,"m_finalize_string finalizes the README direct-write example…");
	TEST(test_libmem_0051,"mem_finalize_string can add a terminator and rejects data descriptors…");
	TEST(test_libmem_0070,"mem_formatted_string renders printf-style output for char and wchar_t string descriptors…");

	RETURN_STATUS;
}
