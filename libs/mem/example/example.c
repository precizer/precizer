#include "../src/mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct point {
	int x;
	int y;
} point;

/**
 * @brief Fill the descriptor with sequential point data.
 *
 * @param points Descriptor to populate.
 * @return Return describing success or failure.
 */
static Return fill_points(memory *points)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;
	point *typed_points = data(point,points);

	if(typed_points == NULL){
		fprintf(stderr,"data returned NULL while filling points\n");
		status = FAILURE;
	}

	if(SUCCESS == status){
		for(size_t point_index = 0; point_index < points->length; ++point_index){
			typed_points[point_index] = (point){
				(int)(point_index * 2 + 1),
				(int)(point_index * 2 + 2)
			};
		}
	}

	provide(status);
}

/**
 * @brief Test numeric descriptors: resize, copy, append, and pointer helpers.
 *
 * @return Return describing success or failure.
 */
static Return test_point_memory(void)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	create(point,points);
	create(point,mirror);
	create(point,zeroed_points);

	run(memory_verify_type(points,sizeof(point)));
	run(memory_verify_type(mirror,sizeof(point)));
	run(memory_verify_type(zeroed_points,sizeof(point)));

	run(resize(points,5));
	run(resize(zeroed_points,3,ZERO_NEW_MEMORY));

	if(SUCCESS == status){
		const point *zeroed_view = cdata(point,zeroed_points);
		if(zeroed_view == NULL){
			fprintf(stderr,"cdata returned NULL for zeroed_points\n");
			status = FAILURE;
		} else if((zeroed_view[0].x != 0) || (zeroed_view[0].y != 0)){
			fprintf(stderr,"ZERO_NEW_MEMORY did not clear newly allocated bytes\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		point *zeroed_writer = data(point,zeroed_points);
		if(zeroed_writer == NULL){
			fprintf(stderr,"data returned NULL for zeroed_points\n");
			status = FAILURE;
		} else {
			zeroed_writer[0] = (point){1,1};
		}
	}

	if(SUCCESS == status){
		run(resize(zeroed_points,6,ZERO_NEW_MEMORY | RELEASE_UNUSED));
	}

	if(SUCCESS == status){
		const point *expanded_view = cdata(point,zeroed_points);
		if(expanded_view == NULL){
			fprintf(stderr,"cdata returned NULL after combined growth\n");
			status = FAILURE;
		} else if((expanded_view[5].x != 0) || (expanded_view[5].y != 0)){
			fprintf(stderr,"Combined ZERO_NEW_MEMORY flag failed to clear new bytes\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		run(resize(zeroed_points,2,ZERO_NEW_MEMORY | RELEASE_UNUSED));
	}

	if(SUCCESS == status){
		const point *shrunk_view = cdata(point,zeroed_points);
		if(shrunk_view == NULL){
			fprintf(stderr,"cdata returned NULL after combined shrink\n");
			status = FAILURE;
		} else if(zeroed_points->length != 2){
			fprintf(stderr,"RELEASE_UNUSED did not reduce the length as expected\n");
			status = FAILURE;
		} else if((shrunk_view[0].x != 1) || (shrunk_view[0].y != 1)){
			fprintf(stderr,"Shrink should preserve existing payload\n");
			status = FAILURE;
		}
	}

	run(fill_points(points));

	if(SUCCESS == status){
		point *raw_points = (point *)rawdata(points);
		if(raw_points == NULL){
			fprintf(stderr,"rawdata returned NULL for points\n");
			status = FAILURE;
		} else {
			raw_points[0].x += 100;
			raw_points[0].y += 200;
		}
	}

	if(SUCCESS == status){
		const point *readonly_points = (const point *)rawcdata(points);
		if(readonly_points == NULL){
			fprintf(stderr,"rawcdata returned NULL for points\n");
			status = FAILURE;
		} else if((readonly_points[0].x != 101) || (readonly_points[0].y != 202)){
			fprintf(stderr,"rawcdata verification failed for adjusted point\n");
			status = FAILURE;
		}
	}

	run(copy(mirror,points));

	if(SUCCESS == status){
		const point *mirror_points = cdata(point,mirror);
		if(mirror_points == NULL){
			fprintf(stderr,"cdata returned NULL for mirror copy\n");
			status = FAILURE;
		} else if(mirror->length != 5){
			fprintf(stderr,"copy produced unexpected length\n");
			status = FAILURE;
		} else if((mirror_points[0].x != 101) || (mirror_points[0].y != 202)){
			fprintf(stderr,"copy did not duplicate adjusted data\n");
			status = FAILURE;
		}
	}

	run(resize(points,8));
	if(SUCCESS == status){
		point *writable_points = data(point,points);
		if(writable_points == NULL){
			fprintf(stderr,"data returned NULL for writable points\n");
			status = FAILURE;
		} else {
			writable_points[5] = (point){11,12};
			writable_points[6] = (point){13,14};
			writable_points[7] = (point){15,16};
		}
	}
	if(SUCCESS == status){
		run(resize(points,4,RELEASE_UNUSED));
	}

	if(SUCCESS == status){
		if(points->length != 4){
			fprintf(stderr,"Shrink-enabled resize produced unexpected length\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		run(resize(points,8));
	}

	if(SUCCESS == status){
		point *regrown_points = data(point,points);
		if(regrown_points == NULL){
			fprintf(stderr,"data returned NULL after regrowing points\n");
			status = FAILURE;
		} else {
			regrown_points[4] = (point){9,10};
			regrown_points[5] = (point){11,12};
			regrown_points[6] = (point){13,14};
			regrown_points[7] = (point){15,16};
		}
	}

	run(append(mirror,points));

	if(SUCCESS == status){
		const size_t expected_length = 13;
		const point *mirror_view = cdata(point,mirror);
		if(mirror_view == NULL){
			fprintf(stderr,"cdata returned NULL after append\n");
			status = FAILURE;
		} else if(mirror->length != expected_length){
			fprintf(stderr,"append produced unexpected length\n");
			status = FAILURE;
		} else if((mirror_view[expected_length - 1].x != 15) || (mirror_view[expected_length - 1].y != 16)){
			fprintf(stderr,"append lost the tail element\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		printf("Point descriptor test passed.\n");
	}

	const Return test_result_status = status;

	status = SUCCESS;
	run(del(points));
	const Return cleanup_points_status = status;

	status = SUCCESS;
	run(del(mirror));
	const Return cleanup_mirror_status = status;

	status = SUCCESS;
	run(del(zeroed_points));
	const Return cleanup_zeroed_status = status;

	status = test_result_status;
	if(status == SUCCESS){
		if(cleanup_points_status != SUCCESS){
			status = cleanup_points_status;
		} else if(cleanup_mirror_status != SUCCESS){
			status = cleanup_mirror_status;
		} else if(cleanup_zeroed_status != SUCCESS){
			status = cleanup_zeroed_status;
		}
	}

	provide(status);
}

/**
 * @brief Test string helpers: copy_literal, concat_literal, concat_strings.
 *
 * @return Return describing success or failure.
 */
static Return test_string_memory(void)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	create(char,string_buffer);
	create(char,extra_buffer);
	create(char,guarded_buffer);
	const char *final_string_view = NULL;
	const char bounded_gamma[] = {'-','g','a','m','m','a','\0','x'};

	run(copy_literal(string_buffer,"alpha"));
	run(concat_literal(string_buffer,"-beta"));
	run(copy_cstring(extra_buffer,bounded_gamma,sizeof(bounded_gamma)));
	run(concat_strings(string_buffer,extra_buffer));
	run(copy_literal(extra_buffer,"+delta"));
	run(concat_strings(string_buffer,extra_buffer));

	const char bounded_suffix[] = {'-','e','p','s','i','l','o','n','\0','x','x'};
	run(concat_cstring(string_buffer,bounded_suffix,sizeof(bounded_suffix)));

	if(SUCCESS == status){
		const char *string_view = cdata(char,string_buffer);
		if(string_view == NULL){
			fprintf(stderr,"cdata returned NULL for string_buffer\n");
			status = FAILURE;
		} else if(strcmp(string_view,"alpha-beta-gamma+delta-epsilon") != 0){
			fprintf(stderr,"String concatenation result mismatch: %s\n",string_view);
			status = FAILURE;
		}
	}

	size_t previous_string_length = 0;

	if(SUCCESS == status){
		previous_string_length = string_buffer->length;
		run(resize(string_buffer,previous_string_length + 8,ZERO_NEW_MEMORY));
	}

	if(SUCCESS == status){
		const char *expanded_view = cdata(char,string_buffer);
		if(expanded_view == NULL){
			fprintf(stderr,"cdata returned NULL after ZERO_NEW_MEMORY resize\n");
			status = FAILURE;
		} else {
			bool zero_tail = true;
			for(size_t index = previous_string_length; index < string_buffer->length; ++index){
				if(expanded_view[index] != '\0'){
					zero_tail = false;
					break;
				}
			}

			if(!zero_tail){
				fprintf(stderr,"ZERO_NEW_MEMORY did not clear the extended string tail\n");
				status = FAILURE;
			}
		}
	}

	const size_t trimmed_string_length = 6;

	if(SUCCESS == status){
		run(resize(string_buffer,trimmed_string_length,RELEASE_UNUSED));
	}

	if(SUCCESS == status){
		char *trimmed_view = data(char,string_buffer);
		if(trimmed_view == NULL){
			fprintf(stderr,"data returned NULL after RELEASE_UNUSED resize\n");
			status = FAILURE;
		} else {
			if(trimmed_string_length > 0){
				trimmed_view[trimmed_string_length - 1] = '\0';
			}

			if(strcmp(trimmed_view,"alpha") != 0){
				fprintf(stderr,"RELEASE_UNUSED example produced unexpected string: %s\n",trimmed_view);
				status = FAILURE;
			}
		}
	}

	if(SUCCESS == status){
		char *safe_writer = getstring(string_buffer);
		if(safe_writer == NULL){
			fprintf(stderr,"getstring returned NULL for string_buffer\n");
			status = FAILURE;
		} else {
			strcpy(safe_writer,"omega");

			const char *safe_reader = getcstring(string_buffer);
			if(safe_reader == NULL){
				fprintf(stderr,"getcstring returned NULL for string_buffer\n");
				status = FAILURE;
			} else if(strcmp(safe_reader,"omega") != 0){
				fprintf(stderr,"getstring/getcstring produced mismatch: %s\n",safe_reader);
				status = FAILURE;
			} else {
				final_string_view = safe_reader;
			}
		}
	}

	size_t measured_string_length = 0;

	if((SUCCESS == status) && (final_string_view != NULL)){
		run(string_length(string_buffer,&measured_string_length));
	}

	if(SUCCESS == status){
		if(final_string_view == NULL){
			fprintf(stderr,"final_string_view is NULL, cannot validate string_length\n");
			status = FAILURE;
		} else if(measured_string_length != strlen(final_string_view)){
			fprintf(stderr,"string_length mismatch: %zu vs %zu\n",measured_string_length,strlen(final_string_view));
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		const char *empty_guarded = getcstring(guarded_buffer);
		if(empty_guarded == NULL){
			fprintf(stderr,"getcstring returned NULL for guarded_buffer\n");
			status = FAILURE;
		} else if(strlen(empty_guarded) != 0){
			fprintf(stderr,"getcstring fallback should be empty for guarded_buffer\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		run(resize(guarded_buffer,3));
	}

	if(SUCCESS == status){
		char *guarded_writer = data(char,guarded_buffer);
		if(guarded_writer == NULL){
			fprintf(stderr,"data returned NULL while preparing guarded_buffer\n");
			status = FAILURE;
		} else {
			guarded_writer[0] = 'x';
			guarded_writer[1] = 'y';
			guarded_writer[2] = 'z';
		}
	}

	if(SUCCESS == status){
		char *safe_guarded = getstring(guarded_buffer);
		if(safe_guarded == NULL){
			fprintf(stderr,"getstring returned NULL for guarded_buffer\n");
			status = FAILURE;
		} else if(guarded_buffer->length != 3){
			fprintf(stderr,"guarded_buffer changed length unexpectedly: %zu\n",guarded_buffer->length);
			status = FAILURE;
		} else if(safe_guarded[0] != '\0'){
			fprintf(stderr,"getstring failed to patch a missing terminator in guarded_buffer\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		printf("String descriptor test passed.\n");
	}

	const Return test_result_status = status;

	status = SUCCESS;
	run(del(string_buffer));
	const Return cleanup_string_status = status;

	status = SUCCESS;
	run(del(extra_buffer));
	const Return cleanup_extra_status = status;

	status = SUCCESS;
	run(del(guarded_buffer));
	const Return cleanup_guarded_status = status;

	status = test_result_status;
	if(status == SUCCESS){
		if(cleanup_string_status != SUCCESS){
			status = cleanup_string_status;
		} else if(cleanup_extra_status != SUCCESS){
			status = cleanup_extra_status;
		} else if(cleanup_guarded_status != SUCCESS){
			status = cleanup_guarded_status;
		}
	}

	provide(status);
}

/**
 * @brief Test copy_buffer helper with known-size arrays.
 *
 * @return Return describing success or failure.
 */
static Return test_copy_buffer_memory(void)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	create(char,db_path);
	create(point,point_buffer);

	const char in_memory_db_path[] = ":memory:";
	const point sample_points[] = {
		{3,4},
		{5,6}
	};

	run(copy_buffer(db_path,in_memory_db_path,sizeof(in_memory_db_path)));

	if(SUCCESS == status){
		if(db_path->length != sizeof(in_memory_db_path)){
			fprintf(stderr,"copy_buffer produced unexpected db_path length: %zu\n",db_path->length);
			status = FAILURE;
		} else if(strcmp(getcstring(db_path),":memory:") != 0){
			fprintf(stderr,"copy_buffer produced unexpected string payload: %s\n",getcstring(db_path));
			status = FAILURE;
		}
	}

	run(copy_buffer(point_buffer,sample_points,sizeof(sample_points)));

	if(SUCCESS == status){
		const point *point_view = cdata(point,point_buffer);
		if(point_view == NULL){
			fprintf(stderr,"cdata returned NULL for point_buffer after copy_buffer\n");
			status = FAILURE;
		} else if(point_buffer->length != 2){
			fprintf(stderr,"copy_buffer produced unexpected point length: %zu\n",point_buffer->length);
			status = FAILURE;
		} else if(point_view[0].x != 3 || point_view[0].y != 4 || point_view[1].x != 5 || point_view[1].y != 6){
			fprintf(stderr,"copy_buffer did not preserve point payload\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		run(copy_buffer(point_buffer,NULL,0));
		if(point_buffer->length != 0){
			fprintf(stderr,"copy_buffer(NULL,0) should clear the descriptor\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status){
		printf("copy_buffer helper test passed.\n");
	}

	const Return test_result_status = status;

	status = SUCCESS;
	run(del(db_path));
	const Return cleanup_db_path_status = status;

	status = SUCCESS;
	run(del(point_buffer));
	const Return cleanup_point_buffer_status = status;

	status = test_result_status;
	if(status == SUCCESS){
		if(cleanup_db_path_status != SUCCESS){
			status = cleanup_db_path_status;
		} else if(cleanup_point_buffer_status != SUCCESS){
			status = cleanup_point_buffer_status;
		}
	}

	provide(status);
}

/**
 * @brief Exercise the pointer reset helper for legacy allocations.
 *
 * @return Return describing success or failure.
 */
static Return test_pointer_reset(void)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;
	char *manual_buffer = NULL;

	if(SUCCESS == status){
		manual_buffer = (char *)malloc(64u);
		if(manual_buffer == NULL){
			fprintf(stderr,"malloc failed in test_pointer_reset\n");
			status = FAILURE;
		} else {
			strcpy(manual_buffer,"temporary heap payload");
		}
	}

	if(manual_buffer != NULL){
		reset(&manual_buffer);
	}

	if(SUCCESS == status){
		if(manual_buffer != NULL){
			fprintf(stderr,"reset helper failed to nullify pointer\n");
			status = FAILURE;
		} else {
			printf("Raw pointer reset helper test passed.\n");
		}
	}

	provide(status);
}

/**
 * @brief Run the full test suite for the mem helper.
 *
 * @return Return describing success or failure.
 */
static Return run_mem_tests(void)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	run(test_point_memory());
	run(test_string_memory());
	run(test_copy_buffer_memory());
	run(test_pointer_reset());

	provide(status);
}

/**
 * @brief Entry point that executes the mem tests.
 *
 * @return Zero on success, non-zero otherwise.
 */
int main(void)
{
	init_telemetry();

	const Return status = run_mem_tests();

	telemetry_show();

	if(status == SUCCESS){
		printf("All mem tests passed.\n");
		return 0;
	} else {
		fprintf(stderr,"Mem tests failed.\n");
		return 1;
	}
}
