#include "precizer.h"

/**
 * @brief Build a relative path by trimming a known root prefix from a bounded path
 *
 * Stores @p path relative to @p root_path in @p relative_path. If @p path is not
 * inside @p root_path, the original path text is copied. If both paths point to
 * the same location, the result is "."
 *
 * @param[in,out] relative_path Destination descriptor that receives the relative path
 * @param[in] path Absolute or relative path to inspect
 * @param[in] path_length Path length in bytes, without the trailing `'\0'`
 * @param[in] root_path Non-NULL root-path descriptor to remove from the
 *            beginning of @p path
 * @return SUCCESS on success. Returns FAILURE when required input pointers are NULL
 *         or when the destination descriptor cannot accept the copied result
 */
Return extract_relative_path(
	memory       *relative_path,
	const char   *path,
	const size_t path_length,
	const memory *root_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* The function starts with the safest fallback result: copy the original
	   input path unchanged. Later blocks move this pointer only after proving
	   that root_path is a real path prefix, not just a similar string prefix */
	const char *relative_path_begin = path;

	/* m_copy_fixed_string() expects the complete source size, including the
	   trailing '\0'. For the fallback result, that is the supplied path length
	   plus one byte for the terminator */
	size_t relative_path_size = path_length + 1U;

	/* Validate mandatory inputs before any pointer arithmetic or descriptor
	   access. Each check returns immediately through provide(), so later code
	   can assume that these pointers exist */
	if(relative_path == NULL)
	{
		report("Path trimming; Destination descriptor must be non-NULL");
		provide(FAILURE);
	}

	if(path == NULL)
	{
		report("Path trimming; Source path must be non-NULL");
		provide(FAILURE);
	}

	if(root_path == NULL)
	{
		report("Path trimming; Root path descriptor must be non-NULL");
		provide(FAILURE);
	}

	/* Keep a pointer to the visible end of path. This makes later size math
	   independent of strlen(), so the caller-provided bounded length remains
	   authoritative */
	const char *path_terminator = path + path_length;

	/* An empty root_path cannot describe a path component prefix. In that case
	   the fallback result remains active and the original path is copied */
	if(root_path->string_length > 0U)
	{
		/* Get a read-only view of the root descriptor. A NULL view means the
		   descriptor is not currently readable as a byte string, so trimming is
		   skipped and the original path remains the result */
		const char *runtime_root_path = m_data_ro(char,root_path);

		/* All prefix checks below are meaningful only when the root descriptor
		   has a readable byte-string view */
		if(runtime_root_path != NULL)
		{
			/* string_length is the visible root text length, excluding the
			   descriptor's trailing '\0'. That is exactly the number of bytes
			   compared against the beginning of path */
			size_t root_path_len = root_path->string_length;

			/* First check only whether path starts with root_path. This is not
			   enough by itself: /home/user2 starts with /home/user, but it is
			   not inside /home/user. The boundary checks inside this block make
			   that distinction */
			if(root_path_len <= path_length &&
			        strncmp(path,runtime_root_path,root_path_len) == 0)
			{
				/* If root_path already ends with a separator, the separator is
				   part of the root itself. This covers roots like "/" and "./",
				   where the next byte in path is normally the first byte of the
				   relative name rather than another separator */
				const bool root_path_ends_with_separator =
				        runtime_root_path[root_path_len - 1U] == '/' ||
				        runtime_root_path[root_path_len - 1U] == '\\';

				/* An exact match means path and root_path name the same place.
				   The result should become "." after the trim leaves an empty
				   visible path */
				const bool path_matches_root_exactly = root_path_len == path_length;

				/* This starts false so exact matches do not read path[root_path_len],
				   which would be the terminator byte outside the visible path */
				bool path_continues_with_separator = false;

				/* When path continues after root_path, the next byte must be a path
				   separator. Without this check, /home/user2/file would be wrongly
				   treated as a child of /home/user and become 2/file */
				if(path_matches_root_exactly == false)
				{
					path_continues_with_separator =
					        path[root_path_len] == '/' ||
					        path[root_path_len] == '\\';
				}

				/* Now the prefix is known to be a real path boundary:
				   - the root itself ends with a separator, such as "/" or "./"
				   - or path is exactly root_path
				   - or path continues with a separator after root_path
				   If none of these is true, the fallback original path is kept */
				if(root_path_ends_with_separator ||
				        path_matches_root_exactly ||
				        path_continues_with_separator)
				{
					/* Move the output start just after the root text. This may
					   point at a separator, the path terminator, or the first
					   byte of a relative name when root_path ended with a separator */
					relative_path_begin = path + root_path_len;

					/* For roots that do not include their trailing separator,
					   skip exactly one separator between root_path and the child
					   component. This turns /root/file into file */
					if(relative_path_begin < path_terminator &&
					        (*relative_path_begin == '/' || *relative_path_begin == '\\'))
					{
						relative_path_begin++;
					}

					/* Recalculate the fixed-string source size from the new
					   start pointer to one byte past the original terminator.
					   If start is the terminator, the size becomes 1 and the
					   final block will store "." instead of an empty path */
					relative_path_size = (size_t)((path_terminator + 1U) - relative_path_begin);
				}
			}
		}
	}

	/* Copy the selected result into the caller-owned descriptor. A size of one
	   means the selected source would be only "\0"; represent that exact-root
	   case as "." so callers get a usable relative path */
	if(relative_path_size == 1U)
	{
		run(m_copy_literal(relative_path,"."));
	} else {
		run(m_copy_fixed_string(relative_path,relative_path_size,relative_path_begin));
	}

	provide(status);
}
