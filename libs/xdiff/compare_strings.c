#include "xdiff.h"

/**
 * @brief Structure for storing output buffer
 */
typedef struct {
	char *buffer;   /**< Pointer to data buffer */
	size_t size;    /**< Current size of data in buffer */
	size_t capacity;        /**< Total buffer capacity */
} output_buffer_t;

/**
 * @brief Initializes output buffer for storing comparison results
 *
 * @return output_buffer_t* Pointer to created buffer or NULL on error
 */
static output_buffer_t *init_output_buffer(void){
	Return status = SUCCESS;
	output_buffer_t *buf = NULL;

	/* Allocate memory for buffer structure */
	buf = (output_buffer_t *)malloc(sizeof(output_buffer_t));

	if(NULL == buf)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Set initial size */
		buf->capacity = 1024;
		buf->buffer = (char *)malloc(buf->capacity);

		if(NULL == buf->buffer)
		{
			free(buf);
			buf = NULL;
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		buf->size = 0;
		buf->buffer[0] = '\0';
	}

	return buf;
}

/**
 * @brief Appends data to output buffer
 *
 * @param priv Pointer to buffer (output_buffer_t*)
 * @param mb Array of buffers with data
 * @param nbuf Number of buffers
 * @return Return Operation status
 */
static int append_to_buffer(
	void       *priv,
	mmbuffer_t *mb,
	int        nbuf
){
	int status = SUCCESS;
	output_buffer_t *buf = (output_buffer_t *)priv;
	size_t total_size = 0;
	size_t new_capacity = 0;
	char *new_buffer = NULL;

	/* Check input parameters */
	if(NULL == buf || NULL == mb)
	{
		return(1);
	}

	/* Calculate total size to append */
	if(SUCCESS == status)
	{
		for(int i = 0; i < nbuf; i++)
		{
			total_size += mb[i].size;
		}
	}

	/* Check if buffer needs to be enlarged */
	if(SUCCESS == status)
	{
		if(buf->size + total_size + 1 > buf->capacity)
		{
			new_capacity = buf->capacity;

			while(new_capacity < buf->size + total_size + 1)
			{
				new_capacity *= 2;
			}

			new_buffer = (char *)realloc(buf->buffer,new_capacity);

			if(NULL == new_buffer)
			{
				status = FAILURE;
			} else {
				buf->buffer = new_buffer;
				buf->capacity = new_capacity;
			}
		}
	}

	/* Copy data */
	if(SUCCESS == status)
	{
		for(int i = 0; i < nbuf; i++)
		{
			memcpy(buf->buffer + buf->size,mb[i].ptr,mb[i].size);
			buf->size += mb[i].size;
		}
		buf->buffer[buf->size] = '\0';
	}

	return(status);
}

/**
 * @brief Creates mmfile from string
 *
 * @param mmf Pointer to mmfile_t structure to fill
 * @param content Source string
 * @return Return Operation status
 */
static Return create_mmfile(
	mmfile_t   *mmf,
	const char *content
){
	Return status = SUCCESS;
	size_t size = 0;
	void *data = NULL;

	if(NULL == mmf || NULL == content)
	{
		return(FAILURE);
	}

	size = strlen(content);

	/* Initialize mmfile */
	if(SUCCESS == status)
	{
		if(xdl_init_mmfile(mmf,size + 1,XDL_MMF_ATOMIC) < 0)
		{
			status = FAILURE;
		}
	}

	/* Allocate memory and copy data */
	if(SUCCESS == status)
	{
		data = xdl_mmfile_writeallocate(mmf,size);

		if(NULL == data)
		{
			xdl_free_mmfile(mmf);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		memcpy(data,content,size);
	}

	if(SUCCESS != status)
	{
		serp("Failed to create mmfile");
	}

	return(status);
}

/**
 * @brief Compares two strings and generates diff in unified format
 *
 * @param[out] diff Pointer to string with comparison result
 * @param[in] string1 First string to compare
 * @param[in] string2 Second string to compare
 * @return Return Operation status
 */
Return compare_strings(
	char       **diff,
	const char *string1,
	const char *string2
){
	Return status = SUCCESS;
	mmfile_t mf1,mf2;
	xpparam_t xpp;
	xdemitconf_t xecfg;
	xdemitcb_t ecb;
	output_buffer_t *output = NULL;

	/* Validate input parameters */
	if(NULL == diff || NULL == string1 || NULL == string2)
	{
		return(FAILURE);
	}

	/* Create output buffer */
	if(SUCCESS == status)
	{
		output = init_output_buffer();

		if(NULL == output || output->buffer == NULL)
		{
			serp("Failed to initialize output buffer");
			status = FAILURE;
		}
	}

	/* Initialize parameters */
	if(SUCCESS == status)
	{
		memset(&xpp,0,sizeof(xpp));
		xpp.flags |= XDF_NEED_MINIMAL;
		xecfg.ctxlen = 0;
		xecfg.str_meta = 0;
		ecb.priv = output;
		ecb.outf = append_to_buffer;
	}

	/* Create mmfile structures from input strings */
	if(SUCCESS == status)
	{
		status = create_mmfile(&mf1,string1);
	}

	if(SUCCESS == status)
	{
		status = create_mmfile(&mf2,string2);
	}

	/* Perform comparison */
	if(SUCCESS == status)
	{
		if(xdl_diff(&mf1,&mf2,&xpp,&xecfg,&ecb) < 0)
		{
			serp("Diff failed");
			status = FAILURE;
		}
	}

	/* Save result */
	if(SUCCESS == status)
	{
		*diff = strdup(output->buffer);

		if(NULL == *diff)
		{
			status = FAILURE;
		}
	}

	/* Free resources */
	xdl_free_mmfile(&mf1);
	xdl_free_mmfile(&mf2);

	if(output != NULL)
	{
		if(output->buffer != NULL)
		{
			free(output->buffer);
		}

		free(output);
	}

	return(status);
}
