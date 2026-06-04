#include "precizer.h"

/**
 * @brief Show a checksum-locked unavailable violation for one path
 *
 * @param[in] relative_path Relative path descriptor being reported. Must not be NULL
 * @param[in] access_status Unavailable classification for the locked path
 * @param[in,out] first_iteration Traversal header state or NULL when banner state is unavailable
 * @param[in,out] summary Traversal summary or NULL when summary tracking is unavailable.
 *                        Ignored when first_iteration is NULL
 *
 * @return Return status code:
 *         - SUCCESS: The violation line was printed
 *         - FAILURE: Unsupported access status or summary was omitted while first_iteration was provided
 */
Return show_locked_checksum_unavailable_violation(
	const memory           *relative_path,
	const FileAccessStatus access_status,
	bool                   *first_iteration,
	TraversalSummary       *summary)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *message = NULL;

	if(first_iteration != NULL && summary == NULL)
	{
		provide(FAILURE);
	}

	if(access_status == FILE_NOT_FOUND)
	{
		message = "checksum locked, file disappeared";

	} else if(access_status == FILE_ACCESS_DENIED){
		message = "checksum locked, access denied";

	} else if(access_status == FILE_ACCESS_ERROR){
		message = "checksum locked, access check failed";

	} else {
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const char *runtime_relative_path = m_text(relative_path);

		if(first_iteration != NULL)
		{
			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,
				RED "%s" RESET " %s\n",message,runtime_relative_path);

		} else {
			slog(EVERY|UNDECOR|REMEMBER,RED "%s" RESET " %s\n",message,runtime_relative_path);
		}

		/*
		 * A checksum-locked file that disappears or becomes unreadable is treated as possible data corruption.
		 * Replay the remembered warning at exit even without --progress so this critical result is not lost among traversal or cleanup messages
		 */
		config->show_remembered_messages_at_exit = true;
	}

	provide(status);
}
