#include "rational.h"

#define PRINT_CHECKOUT 0

/**
 *
 * @brief Locale-aware format 1234567.89 -> 1,234,567.89
 * @details va_arg format got from https://stackoverflow.com/a/23647983/7104681
 * and https://stackoverflow.com/questions/1449805/how-to-format-a-number-using-comma-as-thousands-separator-in-c
 * @param val - Any lond double digit
 * @return Pointer to a string
 *
 */
const char *form(
	long double val
)
{
	static char result[MAX_NUMBER_CHARACTERS];

	// Compare the argument passed to the parent function
	// with zero accurate to 10 digits and output zero
	if(fabsl(val) < 0.0000000001L){
		result[0]='0';
		result[1]='\0';
		return result;
	}

	snprintf(result, sizeof(result),"%.9Lf",val);
	char *pt = NULL;
	// Point pt at "."
	for (pt = result; *pt && *pt != '.'; pt++){
		#if PRINT_CHECKOUT
		printf("pt: %p, pt addr: %zu\n",(void*)pt,(void*)pt);
		#endif
	}

	/*
	 * Adding a thousand separator
	 *
	 */

	// Length of fractional part
	size_t n = (size_t)result + sizeof(result) - (size_t)pt - 1;
	#if PRINT_CHECKOUT
	printf("n: %zu\n",n);
	#endif

	// Step backwards, inserting spaces
	do{
		pt -= 3; // shift 3 digits
		if (pt > result && *(pt-1) != '-'){
		//              ^______ to prevent space between  munus and other digits
			memmove(pt + 1, pt, n);
			#if PRINT_CHECKOUT
			printf("pt: %p, pt addr: %zu\n",(void*)pt,(void*)pt);
			#endif
			// memmove allows overlap, unlike memcpy
			*pt = ','; // thousand separator
			n += 4; // 3 digits + separator
		}else{
			break;
		}
	}while(true);

	#if PRINT_CHECKOUT
	printf("n: %zu\n",n);
	#endif

	char *ptr=strchr(result,'.');
	#if PRINT_CHECKOUT
	printf("ptr: %p, ptr addr: %zu\n",(void*)ptr,(void*)ptr);
	#endif
	if(ptr!=NULL){
		// Either val is real or interger
		bool val_is_real = false;
		for(pt = ptr+1; pt < (result + sizeof(result)) && *pt != '\0'; pt++){
			if(*pt != '0'){
				val_is_real = true;
				break;
			}
		}
		if(val_is_real){
			*ptr='.';
			// Remove unnecessary terminated zeroes
			for(pt = (result + sizeof(result)) - 1;pt >= ptr;--pt){
				#if PRINT_CHECKOUT
				printf("pt: %p, pt addr: %zu, ptr: %p, ptr addr: %zu\n",(void*)pt,(void*)pt,(void*)ptr,(void*)ptr);
				#endif
				if(*pt == '\0' || *pt == '0' || *pt == ','){
					*pt = '\0';
				}else{
					break;
				}
			}
			#if 0
			printf("\n");
			#endif
		}else{
			*ptr='\0';
		}
	}

	return(result);
}

// Test
#if 0
int main(void){
	double digit = 837452834.94;
	printf("digit: %s\n",form(digit));
}
#endif
