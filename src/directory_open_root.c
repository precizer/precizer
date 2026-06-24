#include "precizer.h"
#include <errno.h>

/**
 * @brief Open a traversal root and remember an opening failure
 *
 * @details
 * Opens a root directory that will be used as the base for traversal-relative
 * access checks. When the root cannot be opened, the function prints one
 * plain warning immediately and saves the same warning for the final
 * "Warnings and errors encountered" summary
 *
 * @param[in] root_path Descriptor containing the traversal root path
 * @param[out] root_directory_fd_out Receives the opened descriptor
 * @return Status returned by directory_open()
 */
FileAccessStatus directory_open_root(
	const memory *root_path,
	int          *root_directory_fd_out)
{
	const FileAccessStatus root_access_status = directory_open(root_path,root_directory_fd_out);

	if(root_access_status != FILE_ACCESS_ALLOWED)
	{
		const int root_open_errno = errno;

		slog(EVERY|UNDECOR|REMEMBER,
			"Unable to open traversal root %s: %s\n",
			m_text(root_path),
			strerror(root_open_errno));

		/*
		 * The caller may skip this root immediately, before later traversal
		 * code has a chance to request the final warning summary. Enable
		 * that summary here so the message saved by REMEMBER is shown again
		 * at exit
		 */
		config->show_remembered_messages_at_exit = true;
	}

	return(root_access_status);
}
