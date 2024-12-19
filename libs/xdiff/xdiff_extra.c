#include "xdiff.h"

/**
 * @brief Structure to store callback data for diff output
 */
typedef struct {
	char const *prefix; /**< Prefix for output lines */
	FILE *output_stream;   /**< Output stream (e.g. stdout) */
} diff_output_t;

/**
 * @brief Callback function for diff output
 * @param priv Private data (diff_output_t structure)
 * @param mb Array of buffers to output
 * @param nb Number of buffers
 * @return 0 on success, -1 on failure
 */
static int diff_output_callback(
	void       *priv,
	mmbuffer_t *mb,
	int        nb
){
	diff_output_t *diff_data = (diff_output_t *)priv;

	for(int i = 0; i < nb; i++)
	{
		if(fwrite(mb[i].ptr,mb[i].size,1,diff_data->output_stream) != 1)
		{
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Custom memory allocator functions
 */
static void *custom_malloc(
	void         *priv,
	unsigned int size
){
	return malloc(size);
}

static void custom_free(
	void *priv,
	void *ptr
){
	free(ptr);
}

static void *custom_realloc(
	void         *priv,
	void         *ptr,
	unsigned int size
){
	return realloc(ptr,size);
}

/**
 * @brief Initialize memory allocator
 * @return Return status
 */
static Return init_allocator(void){
	Return status = SUCCESS;
	memallocator_t malt;

	malt.priv = NULL;
	malt.malloc = custom_malloc;
	malt.free = custom_free;
	malt.realloc = custom_realloc;

	if(xdl_set_allocator(&malt) < 0)
	{
		status = FAILURE;
	}

	return status;
}

/**
 * @brief Initialize mmfile structure with text data
 * @param mmf Pointer to mmfile structure
 * @param text Text data
 * @return Return status
 */
static Return init_mmfile(
	mmfile_t   *mmf,
	char const *text
){
	Return status = SUCCESS;
	long text_size;
	void *buffer;

	if(xdl_init_mmfile(mmf,XDLT_STD_BLKSIZE,XDL_MMF_ATOMIC) < 0)
	{
		return FAILURE;
	}

	text_size = strlen(text);

	buffer = xdl_mmfile_writeallocate(mmf,text_size);

	if(!buffer)
	{
		xdl_free_mmfile(mmf);
		return FAILURE;
	}

	memcpy(buffer,text,text_size);

	return status;
}

/**
 * @brief Compare two text arrays and output differences
 * @param text First text array
 * @param compare Second text array
 * @return Return status
 */
Return compare_texts(
	char const *text,
	char const *compare
){
	Return status = SUCCESS;
	mmfile_t mmf1,mmf2;
	xpparam_t xpp;
	xdemitconf_t xecfg;
	xdemitcb_t ecb;
	diff_output_t diff_data;

	if(SUCCESS != init_allocator())
	{
		return FAILURE;
	}

	if(SUCCESS != init_mmfile(&mmf1,text))
	{
		return FAILURE;
	}

	if(SUCCESS != init_mmfile(&mmf2,compare))
	{
		xdl_free_mmfile(&mmf1);
		return FAILURE;
	}

	/* Initialize diff parameters */
	xpp.flags = 0;
	xecfg.ctxlen = 3; // Context length of 3 lines

	/* Initialize output callback */
	diff_data.prefix = "";
	diff_data.output_stream = stdout;
	ecb.priv = &diff_data;
	ecb.outf = diff_output_callback;

	/* Perform diff operation */
	if(xdl_diff(&mmf1,&mmf2,&xpp,&xecfg,&ecb) < 0)
	{
		status = FAILURE;
	}

	/* Cleanup */
	xdl_free_mmfile(&mmf2);
	xdl_free_mmfile(&mmf1);

	return status;
}

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef enum {
	SUCCESS = 0,
	FAILURE = 1,
	WARNING = 2,
	DONOTHING = 3
} Return;

static size_t read_file_to_array(
	const char *filename,
	char       **array
){
	FILE *file = fopen(filename,"rb");

	if(!file)
	{
		return 0;
	}

	// Получаем размер файла
	struct stat st;

	if(stat(filename,&st) != 0)
	{
		fclose(file);
		return 0;
	}

	// Выделяем память с учетом нуль-терминатора
	*array = (char *)malloc(st.st_size + 1);

	if(!*array)
	{
		fclose(file);
		return 0;
	}

	// Читаем файл
	size_t bytes_read = fread(*array,1,st.st_size,file);
	fclose(file);

	if(bytes_read != st.st_size)
	{
		free(*array);
		*array = NULL;
		return 0;
	}

	// Добавляем нуль-терминатор
	(*array)[bytes_read] = '\0';

	return bytes_read;
}

int main(
	int  argc,
	char *argv[]
){

	char *text1 = NULL;
	size_t bytes_read = read_file_to_array(argv[1],&text1);

	char *text2 = NULL;
	bytes_read = read_file_to_array(argv[2],&text2);

	if(SUCCESS != compare_texts(text1,text2))
	{
		fprintf(stderr,"Error comparing texts\n");
		return 1;
	}

	free(text1);
	free(text2);

	return 0;
}
#endif
