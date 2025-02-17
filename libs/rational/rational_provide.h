
const char *return_status(const Return) __attribute__((const));

#define provide(status) \
	if (SUCCESS != status) \
	{ \
		slog(TRACE,"Returned %s:%d status: %s\n", __func__, __LINE__,return_status(status)); \
	} \
	return(status);

