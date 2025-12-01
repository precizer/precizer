
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

// Macro to call a function only if the current status is SUCCESS.
// If the status is SUCCESS, the function's return value will be assigned to status.
#define run(func) \
	if(SUCCESS == status) \
	{ \
		status = (func); \
	}

#define call(func) \
	{ \
		Return __call_status = (func); \
		if(SUCCESS == status && SUCCESS != __call_status) \
		{ \
			status = __call_status; \
		} \
	}
