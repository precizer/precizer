
void provide(const Return);

const char *show_status(const Return) __attribute__((const));

#define provide(status) \
	Return return_value = status; \
         \
	if(global_return_status != SUCCESS) \
	{ \
		return_value = global_return_status; \
	} \
         \
	if(SUCCESS != return_value) \
	{ \
		slog(TRACE,"Returned %s:%d status: %s\n",__func__,__LINE__,show_status(return_value)); \
	} \
         \
	return(return_value);
