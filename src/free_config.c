#include "precizer.h"

/**
 * Function to free the array
 *
 */
STATIC void free_str_array(char **array)
{
	if(array == NULL)
	{
		return;
	}

	for(size_t i = 0; array[i] != NULL; i++)
	{
		free(array[i]);
	}
	free(array);
}

/**
 *
 * Clean up allocated memory
 *
 */
void free_config(void)
{
	/// Enable key echo in terminal (return back
	/// default settings)
	struct termios term;
	tcgetattr(fileno(stdin),&term);
	term.c_lflag |= (ICANON|ECHO);
	tcsetattr(fileno(stdin),0,&term);

	/* Cleanup and close previously used DB */
	if(config->db != NULL)
	{
		/**
		 * @brief Force cache flush to disk for data persistence
		 * @note This is the first approach to ensure data integrity
		 */
		sqlite3_db_cacheflush(config->db);

		/**
		 * @brief Configure SQLite for maximum reliability using PRAGMA
		 * @note This is the second approach to ensure data integrity
		 * @details Sets synchronous mode to FULL for maximum durability
		 */
		sqlite3_exec(config->db,"PRAGMA synchronous = FULL;",NULL,NULL,NULL);

		/**
		 * @brief Close database connection and cleanup resources
		 * @note Must be called to prevent resource leaks
		 */
		sqlite3_close(config->db);
	}

	free(config->running_dir);

	free(config->db_file_path);
	config->db_file_path = NULL;

	free(config->db_file_name);
	config->db_file_name = NULL;

	// Free memory of string array
	free_str_array((config)->db_file_names);

	// Free memory of string array
	free_str_array((config)->ignore);

	// Free memory of string array
	free_str_array((config)->include);
}
