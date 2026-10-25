#include "testitall.h"
#include <wordexp.h>

/**
 * @brief Replace all occurrences of a placeholder in an existing text file
 *
 * The filename and replacement may contain environment variables such as $TMPDIR.
 * Each is quoted internally before wordexp() runs with command substitution
 * disabled. Spaces, literal quotes, backslashes, and wildcard characters are
 * preserved. An empty replacement removes the matched placeholders.
 * The placeholder pattern is passed unchanged to replace_placeholder(). A marker
 * such as %ROOT_PATH% is replaced together with its percent signs.
 * The file is read and the replacement is prepared before opening it for writing.
 * Relative paths are resolved from the current working directory
 *
 * @param[in] filename Expression producing the path to the file to read and overwrite
 * @param[in] placeholder Pattern identifying the placeholder to replace
 * @param[in] replacement Expression producing the replacement text
 * @return SUCCESS when the updated text is written and the file is closed,
 *         FAILURE on an expansion, read, replacement, write, or close error
 */
Return replace_placeholder_in_file(
	const char *filename,
	const char *placeholder,
	const char *replacement)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	const char *expressions[] = {filename,replacement};
	wordexp_t expanded[2] = {0};
	bool words_allocated[2] = {false,false};
	m_create(char,content,MEMORY_STRING);
	m_create(char,quoted_expression,MEMORY_STRING);

	/* Expand the file path and replacement independently, keeping their storage
	   alive until all file operations finish */
	for(size_t i = 0U; i < 2U && SUCCESS == status; ++i)
	{
		if(expressions[i] == NULL)
		{
			echo(STDERR,"File replacement expression must not be NULL\n");
			status = FAILURE;
		}

		/* Keep the complete input in one quoted word while leaving variables
		   available for expansion. Quotes and backslashes belong to the text */
		run(m_copy_literal(quoted_expression,"\""));
		for(const char *cursor = expressions[i]; SUCCESS == status && *cursor != '\0'; ++cursor)
		{
			if(*cursor == '"' || *cursor == '\\')
			{
				run(m_concat_literal(quoted_expression,"\\"));
			}
			const char character[] = {*cursor,'\0'};
			run(m_concat_fixed_string(quoted_expression,sizeof(character),character));
		}
		run(m_concat_literal(quoted_expression,"\""));

		if(SUCCESS == status)
		{
			const int word_status = wordexp(m_text(quoted_expression),&expanded[i],WRDE_NOCMD);
			/* Allocation failure may still leave words that require cleanup */
			words_allocated[i] = word_status == 0 || word_status == WRDE_NOSPACE;
			if(word_status != 0)
			{
				echo(STDERR,"Failed to expand expression \"%s\" (wordexp code %d)\n",
					expressions[i],word_status);
				status = FAILURE;
			} else if(expanded[i].we_wordc != 1U){
				echo(STDERR,"Expected one word from expression \"%s\"\n",expressions[i]);
				status = FAILURE;
			}
		}
	}

	if(SUCCESS == status)
	{
		filename = expanded[0].we_wordv[0];
		replacement = expanded[1].we_wordv[0];
	}

	run(get_file_content(filename,content));
	run(replace_placeholder(content,placeholder,replacement));

	if(SUCCESS == status)
	{
		file = fopen(filename,"wb");
		if(file == NULL)
		{
			echo(STDERR,"Failed to open file for writing: %s\n",filename);
			status = FAILURE;
		}
	}

	if(SUCCESS == status && fputs(m_text(content),file) == EOF)
	{
		echo(STDERR,"Failed to write file: %s\n",filename);
		status = FAILURE;
	}

	if(file != NULL && fclose(file) != 0)
	{
		echo(STDERR,"Failed to close file: %s\n",filename);
		status = FAILURE;
	}

	call(m_del(quoted_expression));
	call(m_del(content));
	for(size_t i = 0U; i < 2U; ++i)
	{
		if(words_allocated[i] == true)
		{
			wordfree(&expanded[i]);
		}
	}
	provide(status);
}
