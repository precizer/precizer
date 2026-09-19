/**
 * @file
 * @brief Literal substring replacement for formatted logger messages
 */

#include "rational.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Replace every non-overlapping occurrence of a literal substring
 *
 * @details Counts matches and calculates the resulting byte length before
 *          changing the text. Shorter or equal replacements reuse the existing
 *          buffer. Longer replacements resize it once and move the source to
 *          the right, leaving room for a forward copy into the same allocation.
 *          Replacement bytes are not searched again during this call.
 *          The caller must supply the exact text length, and the search and
 *          replacement strings must not point into the text buffer
 *
 * @param[in,out] text Pointer to an owned, realloc-compatible, null-terminated
 *                    text buffer; updated if resizing changes its address
 * @param[in,out] text_length Exact byte length, excluding the null terminator;
 *                           updated to the resulting length on success
 * @param[in] search Nonempty literal substring to replace
 * @param[in] replacement Literal replacement, which may be empty
 * @return true on success; false for invalid arguments, size overflow, or an
 *         allocation failure, leaving the original buffer and length unchanged
 */
bool rational_logger_replace_all(
	char       **text,
	size_t     *text_length,
	const char *search,
	const char *replacement)
{
	/* Require valid argument pointers and a nonempty search string. An empty
	   search would not advance the matching loop. A length of SIZE_MAX leaves
	   no room for the terminating null byte, so reject it before arithmetic */
	if(text == NULL || *text == NULL || text_length == NULL
	        || search == NULL || replacement == NULL || search[0] == '\0'
	        || *text_length == SIZE_MAX)
	{
		return(false);
	}

	/* Measure the two literal strings once and retain the input byte length.
	   These lengths determine how much each match grows or shrinks the text */
	const size_t search_length = strlen(search);
	const size_t replacement_length = strlen(replacement);
	const size_t source_length = *text_length;
	size_t match_count = 0U;
	const char *match = *text;

	/* Count matches without changing the text. Continue after the complete
	   matched substring so overlapping occurrences are not counted twice */
	while((match = strstr(match,search)) != NULL)
	{
		match_count++;
		match += search_length;
	}

	/* No matches is a successful result that requires neither copying nor
	   resizing. Leave the caller's buffer pointer and byte length unchanged */
	if(match_count == 0U)
	{
		return(true);
	}

	/* Start with the input length and adjust it by the total size difference
	   contributed by all matches. This length excludes the null terminator */
	size_t result_length = source_length;

	/* A longer replacement adds the same number of bytes for every match.
	   Reserve space for all of that growth before modifying the text */
	if(replacement_length > search_length)
	{
		const size_t growth = replacement_length - search_length;

		/* Reserve one byte for the terminator and check the maximum safe
		   match count by division before multiplying. This prevents overflow
		   in both the resulting length and the allocation size */
		if(match_count > (SIZE_MAX - source_length - 1U) / growth)
		{
			return(false);
		}

		/* Request the complete output size in one realloc call. Keep its
		   result separate from the caller's pointer until success is known */
		result_length += match_count * growth;
		char *resized_text = realloc(*text,result_length + 1U);

		/* A failed realloc leaves the original allocation valid. Return
		   without changing the caller's pointer, text, or recorded length */
		if(resized_text == NULL)
		{
			return(false);
		}

		/* Publish the successful allocation, which may have a new address */
		*text = resized_text;
	} else {
		/* Subtract the bytes removed by all matches. Equal-length replacements
		   subtract zero. Keep the allocated capacity and reuse the same buffer */
		result_length -= match_count * (search_length - replacement_length);
	}

	/* Read the source through one pointer and build the result through another.
	   Both pointers refer to regions within the caller's allocation */
	const char *source = *text;
	char *destination = *text;

	/* Reserve expansion space ahead of the source so unread bytes stay intact.
	   Move the complete input, including its terminator, right by the total
	   growth. memmove handles overlapping regions, and the gap allows output
	   to be written forward without overwriting source bytes still needed */
	if(result_length > source_length)
	{
		const size_t offset = result_length - source_length;
		memmove(*text + offset,*text,source_length + 1U);
		source += offset;
	}

	/* Find each match in the unread source and write the resulting pieces
	   from left to right. Each iteration consumes one complete occurrence */
	while((match = strstr(source,search)) != NULL)
	{
		/* Preserve the ordinary text before the match. The source and output
		   regions can overlap, so copying this prefix requires memmove */
		const size_t prefix_length = (size_t)(match - source);
		memmove(destination,source,prefix_length);
		destination += prefix_length;

		/* Insert the replacement bytes and advance the write position. An
		   empty replacement inserts nothing and therefore removes the match */
		memcpy(destination,replacement,replacement_length);
		destination += replacement_length;

		/* Continue reading after the matched source substring. Inserted
		   replacement bytes are outside the remaining search region */
		source = match + search_length;
	}

	/* Copy the remaining unmatched suffix together with its null terminator.
	   This also finishes the result when the final match ends the input */
	memmove(destination,source,strlen(source) + 1U);

	/* Publish the calculated byte length after the complete result is ready */
	*text_length = result_length;
	return(true);
}

/**
 * @brief Replace supported logger markers with terminal bytes or empty strings
 *
 * @details Applies literal replacements to formatted, sanitized text in table
 *          order. Enabled styling replaces markers with terminal sequences;
 *          disabled styling replaces them with empty strings. With styling
 *          enabled, reset markers insert the terminal reset sequence.
 *          The table's replacement strings are shorter than their markers,
 *          so processing reuses the text buffer without allocating memory
 *
 * @param[in,out] text Pointer to the owned, null-terminated buffer containing
 *                    formatted and sanitized text
 * @param[in,out] text_length Exact byte length, excluding the null terminator;
 *                           updated as markers are replaced
 * @param styles_enabled true replaces markers with terminal bytes; false
 *                       removes them. The caller decides whether styling is allowed
 * @return true when all replacements succeed, including when no markers occur;
 *         false stops processing without undoing earlier replacements
 */
bool rational_logger_markup_replace(
	char   **text,
	size_t *text_length,
	bool   styles_enabled)
{
	/* Pair each literal marker with its terminal sequence. The table holds
	   pointers to fixed strings in static storage for the program's lifetime.
	   Its const entries define the replacement rules shared by all calls */
	static const struct {
		const char *marker;
		const char *replacement;
	} decorations[] = {
		{"@{reset}",RESET},
		{"@{shorttab}",SHORTTAB},
		{"@{bold}",BOLD},
		{"@{black}",BLACK},
		{"@{gray}",GRAY},
		{"@{red}",RED},
		{"@{green}",GREEN},
		{"@{yellow}",YELLOW},
		{"@{blue}",BLUE},
		{"@{magenta}",MAGENTA},
		{"@{cyan}",CYAN},
		{"@{white}",WHITE},
		{"@{boldblack}",BOLDBLACK},
		{"@{boldred}",BOLDRED},
		{"@{boldgreen}",BOLDGREEN},
		{"@{boldyellow}",BOLDYELLOW},
		{"@{boldblue}",BOLDBLUE},
		{"@{boldmagenta}",BOLDMAGENTA},
		{"@{boldcyan}",BOLDCYAN},
		{"@{boldwhite}",BOLDWHITE}
	};

	/* Process every rule in table order; each rule sees the text produced by
	   the preceding replacements. Dividing the array's byte size by one
	   entry's byte size gives the entry count used as the loop limit */
	for(size_t decoration_index = 0U;
	        decoration_index < sizeof(decorations) / sizeof(decorations[0]);
	        decoration_index++)
	{
		/* An empty replacement removes the marker without inserting any bytes.
		   Keep this value when styling is disabled, including for reset markers */
		const char *replacement = "";

		/* When styling is enabled, select this rule's terminal sequence.
		   The styles_enabled argument supplies the caller's decision about
		   whether terminal styling is allowed */
		if(styles_enabled == true)
		{
			replacement = decorations[decoration_index].replacement;
		}

		/* Replace all occurrences of the current marker and update the text's
		   byte length. The table's replacements fit in the existing buffer, so no
		   allocation is needed. A missing marker leaves the text unchanged and
		   counts as a successful replacement step */
		if(rational_logger_replace_all(text,text_length,
			decorations[decoration_index].marker,replacement) == false)
		{
			/* Stop at the first error instead of applying the remaining rules.
			   Changes made by earlier iterations remain in the caller's buffer */
			return(false);
		}
	}

	/* Every rule completed successfully, including rules with no matches.
	   The output parameters contain the processed text and its byte length */
	return(true);
}
