#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

/* A volatile pointer keeps the symbol reference in the linked probe even
   when optimization is enabled. Probes are compiled and linked, never run */
#define SYMBOL(name) \
	"int main(void) { __typeof__(&" #name ") volatile address = &" #name "; return address == 0; }\n"

/* Match the POSIX declarations used by argp-compat.c */
#define POSIX_DECLARATIONS \
	"#undef _GNU_SOURCE\n#undef _POSIX_C_SOURCE\n#define _POSIX_C_SOURCE 200112L\n"

/* Each template setting has one self-contained compilation probe */
static const struct {
	const char *name;
	const char *source;
} checks[] = {
	{"HAVE_SYSEXITS_H","#include <sysexits.h>\nint main(void) { return 0; }\n"},
	{"HAVE_UNISTD_H","#include <unistd.h>\nint main(void) { return 0; }\n"},
	{"HAVE_DECL_PUTC_UNLOCKED","#include <stdio.h>\n" SYMBOL(putc_unlocked)},
	{"HAVE_DECL_FPUTS_UNLOCKED","#include <stdio.h>\n" SYMBOL(fputs_unlocked)},
	{"HAVE_DECL_FWRITE_UNLOCKED","#include <stdio.h>\n" SYMBOL(fwrite_unlocked)},
	{"HAVE_STRERROR_R",POSIX_DECLARATIONS "extern int strerror_r(void);\n" SYMBOL(strerror_r)},
	{"HAVE_DECL_STRERROR_R",POSIX_DECLARATIONS "#include <string.h>\n" SYMBOL(strerror_r)},
	{"HAVE_DECL_STRERROR",POSIX_DECLARATIONS "#include <string.h>\n" SYMBOL(strerror)},
	{"HAVE_DECL_STRERROR_S",POSIX_DECLARATIONS "#include <string.h>\n" SYMBOL(strerror_s)},
	{"HAVE_XASPRINTF","extern char *xasprintf(const char *, ...);\n" SYMBOL(xasprintf)},
	{"HAVE_MEMPCPY","#include <string.h>\n" SYMBOL(mempcpy)},
	{"HAVE_RANDOM","#include <stdlib.h>\n" SYMBOL(random)},
	{"HAVE_SLEEP","#include <unistd.h>\n" SYMBOL(sleep)},
	{"HAVE_STRCASECMP","#include <string.h>\n" SYMBOL(strcasecmp)},
	{"HAVE_STRCHRNUL","#include <string.h>\n" SYMBOL(strchrnul)},
	{"HAVE_STRNDUP","#include <string.h>\n" SYMBOL(strndup)},
	{"HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME","#include <errno.h>\n" SYMBOL(program_invocation_short_name)},
	{"HAVE_DECL_PROGRAM_INVOCATION_NAME","#include <errno.h>\n" SYMBOL(program_invocation_name)},
	{"HAVE_DECL___ARGV","#include <stdlib.h>\n" SYMBOL(__argv)},
	{"HAVE___PROGNAME","extern char *__progname;\n" SYMBOL(__progname)},
	{"HAVE_SSIZE_T","#include <sys/types.h>\nint main(void) { return sizeof(ssize_t) == 0; }\n"}
};

/**
 * @brief Split compiler settings into arguments, removing quotes and escapes
 *
 * Arguments share the supplied writable buffer. Single and double quotes keep
 * spaces inside an argument. Variables, wildcards, and commands are not expanded
 *
 * @param text Writable settings string
 * @param arguments Array with room for at least strlen(text) + 1 new entries
 * @param count Number of arguments already stored in the array
 * @return 0 on success or -1 for an unfinished quote or escape
 */
static int split_arguments(
	char   *text,
	char   **arguments,
	size_t *count)
{
	char *read = text;
	char *write = text;

	while(*read != '\0')
	{
		while(isspace((unsigned char)*read) != 0)
		{
			++read;
		}

		if(*read == '\0')
		{
			break;
		}
		arguments[(*count)++] = write;
		char quote = '\0';

		while(*read != '\0' && (quote != '\0' || isspace((unsigned char)*read) == 0))
		{
			char character = *read++;

			if(character == '\\' && quote != '\'')
			{
				if(*read == '\0')
				{
					return -1;
				}

				if(quote == '"' && strchr("\\\"$`\n",*read) == NULL)
				{
					*write++ = '\\';
				}

				if(*read != '\n')
				{
					*write++ = *read;
				}
				++read;
			} else if((character == '\'' || character == '"') && (quote == '\0' || quote == character)){
				if(quote == '\0')
				{
					quote = character;
				} else {
					quote = '\0';
				}
			} else {
				*write++ = character;
			}
		}

		if(quote != '\0')
		{
			return -1;
		}

		while(isspace((unsigned char)*read) != 0)
		{
			++read;
		}
		*write++ = '\0';
	}
	return 0;
}

/**
 * @brief Include bytes in the configuration signature using FNV-1a
 * @param signature Accumulated signature
 * @param data Bytes to include
 * @param length Number of bytes
 */
static void sign_bytes(
	uint64_t   *signature,
	const void *data,
	size_t     length)
{
	const unsigned char *bytes = data;

	for(size_t index = 0; index < length; ++index)
	{
		*signature ^= bytes[index];
		*signature *= UINT64_C(1099511628211);
	}
}

/**
 * @brief Include a template or generator file in the configuration signature
 * @param signature Accumulated signature
 * @param path File to read
 * @return 0 on success or -1 for a file error
 */
static int sign_file(
	uint64_t   *signature,
	const char *path)
{
	FILE *file = fopen(path,"rb");

	if(file == NULL)
	{
		perror(path);
		return -1;
	}
	unsigned char bytes[4096];
	size_t length;

	while((length = fread(bytes,1,sizeof(bytes),file)) != 0)
	{
		sign_bytes(signature,bytes,length);
	}
	int failed = ferror(file);

	if(fclose(file) != 0)
	{
		failed = 1;
	}

	if(failed != 0)
	{
		perror(path);
		return -1;
	}
	return 0;
}

/**
 * @brief Form a helper file path beside the generated header
 * @param path Destination with PATH_MAX bytes
 * @param header Header output path
 * @param name Helper file name
 * @return 0 on success or -1 when the path is too long
 */
static int build_path(
	char       *path,
	const char *header,
	const char *name)
{
	const char *separator = strrchr(header,'/');
	size_t prefix = 0;

	if(separator != NULL)
	{
		prefix = (size_t)(separator - header) + 1;
	}

	if(prefix + strlen(name) >= PATH_MAX)
	{
		fprintf(stderr,"Configuration output path is too long\n");
		return -1;
	}
	memcpy(path,header,prefix);
	strcpy(path + prefix,name);
	return 0;
}

/**
 * @brief Compile and link a feature probe without a shell
 *
 * GNU declarations match the upstream feature checks unless the probe selects
 * POSIX declarations. Apple availability warnings reject functions unavailable
 * at the requested deployment target. Compiler output remains in configure.log
 *
 * @param arguments Compiler argument array, terminated by NULL
 * @param actions Compiler output redirection
 * @param path Probe source path
 * @param source C source for the feature being checked
 * @return 1 for an available feature, 0 for a rejected probe, or -1 for an I/O or process error
 */
static int compile_check(
	char                             **arguments,
	const posix_spawn_file_actions_t *actions,
	const char                       *path,
	const char                       *source)
{
	FILE *file = fopen(path,"w");

	if(file == NULL)
	{
		perror(path);
		return -1;
	}
	int written = fprintf(file,
		"#undef _GNU_SOURCE\n#define _GNU_SOURCE 1\n"
		"#if defined(__APPLE__) && defined(__clang__)\n"
		"#pragma clang diagnostic error \"-Wunguarded-availability\"\n"
		"#pragma clang diagnostic error \"-Wunguarded-availability-new\"\n"
		"#endif\n%s",source);
	int closed = fclose(file);

	if(written < 0 || closed != 0)
	{
		perror(path);
		return -1;
	}

	pid_t process;
	int error = posix_spawnp(&process,arguments[0],actions,NULL,arguments,environ);

	if(error != 0)
	{
		errno = error;
		perror(arguments[0]);
		return -1;
	}
	int result;
	pid_t waited;

	do
	{
		waited = waitpid(process,&result,0);
	} while(waited == -1 && errno == EINTR);

	if(waited == -1)
	{
		perror("waitpid");
		return -1;
	}

	if(WIFEXITED(result) == 0)
	{
		fprintf(stderr,"Compiler terminated unexpectedly\n");
		return -1;
	}
	return WEXITSTATUS(result) == 0;
}

/**
 * @brief Generate config.h using compiler settings from the environment
 *
 * The arguments name the upstream template and output header. An optional
 * --force repeats the probes even when the saved signature matches. The
 * signature covers the settings, working directory, template, and generator.
 * Compiler arguments are passed directly to posix_spawnp(). Relative include
 * and library paths retain their meaning because the working directory stays
 * unchanged. A baseline probe detects a broken toolchain before feature
 * failures can be mistaken for unavailable functions
 *
 * @param argc Number of command-line arguments
 * @param argv Program path, template, output header, and optional --force
 * @return EXIT_SUCCESS for a current or generated header, or EXIT_FAILURE on error
 */
int main(
	int  argc,
	char **argv)
{
	if((argc != 3 && argc != 4) || (argc == 4 && strcmp(argv[3],"--force") != 0))
	{
		fprintf(stderr,"Usage: %s config.h.in config.h [--force]\n",argv[0]);
		return EXIT_FAILURE;
	}

	/* The mode-specific flags already include CFLAGS in the required order */
	const char *names[] = {"CC","CPPFLAGS","CONFIG_FLAGS","LDFLAGS","USE_LLD","C_STANDARD","MACOSX_DEPLOYMENT_TARGET"};
	const size_t setting_count = sizeof(names) / sizeof(names[0]);
	const char *values[sizeof(names) / sizeof(names[0])];
	size_t text_size = 0;
	uint64_t signature = UINT64_C(14695981039346656037);

	for(size_t index = 0; index < setting_count; ++index)
	{
		values[index] = getenv(names[index]);

		if(values[index] == NULL && strcmp(names[index],"CC") == 0)
		{
			values[index] = "cc";
		}

		if(values[index] == NULL && strcmp(names[index],"CONFIG_FLAGS") == 0)
		{
			values[index] = getenv("CFLAGS");
		}

		if(values[index] == NULL && strcmp(names[index],"C_STANDARD") == 0)
		{
			values[index] = "-std=gnu2x";
		}

		if(values[index] == NULL)
		{
			values[index] = "";
		}
		size_t length = strlen(values[index]) + 1;
		sign_bytes(&signature,values[index],length);
		text_size += length;
	}
	char directory[PATH_MAX];

	if(getcwd(directory,sizeof(directory)) == NULL)
	{
		perror("getcwd");
		return EXIT_FAILURE;
	}
	sign_bytes(&signature,directory,strlen(directory) + 1);

	if(sign_file(&signature,argv[1]) != 0 || sign_file(&signature,argv[0]) != 0)
	{
		return EXIT_FAILURE;
	}
	char signature_line[80];
	snprintf(signature_line,sizeof(signature_line),"/* Configuration signature: %016" PRIx64 " */\n",signature);
	FILE *previous = fopen(argv[2],"r");

	if(previous != NULL)
	{
		char line[sizeof(signature_line)];
		bool current = fgets(line,sizeof(line),previous) != NULL && strcmp(line,signature_line) == 0;
		fclose(previous);

		if(current == true && argc == 3)
		{
			return EXIT_SUCCESS;
		}
	}

	char source_path[PATH_MAX],binary_path[PATH_MAX],log_path[PATH_MAX],temporary_path[PATH_MAX];

	if(build_path(source_path,argv[2],"conftest.c") != 0 ||
	        build_path(binary_path,argv[2],"conftest.exe") != 0 ||
	        build_path(log_path,argv[2],"configure.log") != 0 ||
	        build_path(temporary_path,argv[2],"config.h.tmp") != 0)
	{
		return EXIT_FAILURE;
	}

	/* Each argument consumes at least one byte; reserve five probe arguments and NULL */
	char *text = malloc(text_size);
	char **arguments = calloc(text_size + 6,sizeof(*arguments));

	if(text == NULL || arguments == NULL)
	{
		perror("compiler arguments");
		free(text);
		free(arguments);
		return EXIT_FAILURE;
	}
	int status = EXIT_SUCCESS;
	size_t argument_count = 0;
	char *part = text;

	for(size_t index = 0; index + 1 < setting_count && status == EXIT_SUCCESS; ++index)
	{
		size_t length = strlen(values[index]) + 1;
		memcpy(part,values[index],length);

		if(split_arguments(part,arguments,&argument_count) != 0 ||
		        (index == 0 && (argument_count == 0 || arguments[0][0] == '\0')))
		{
			fprintf(stderr,"Invalid compiler setting: %s\n",names[index]);
			status = EXIT_FAILURE;
		}
		part += length;
	}
	arguments[argument_count++] = "-x";
	arguments[argument_count++] = "c";
	arguments[argument_count++] = source_path;
	arguments[argument_count++] = "-o";
	arguments[argument_count++] = binary_path;

	/* Configure output redirection once and reuse it for every compiler invocation */
	posix_spawn_file_actions_t actions;
	int error = posix_spawn_file_actions_init(&actions);
	bool actions_initialized = error == 0;

	if(error == 0)
	{
		error = posix_spawn_file_actions_addopen(&actions,STDOUT_FILENO,log_path,O_WRONLY | O_CREAT | O_APPEND,0666);
	}

	if(error == 0)
	{
		error = posix_spawn_file_actions_adddup2(&actions,STDOUT_FILENO,STDERR_FILENO);
	}

	if(error != 0)
	{
		errno = error;
		perror("compiler output redirection");
		status = EXIT_FAILURE;
	}
	FILE *template = NULL;
	FILE *output = NULL;

	if(status == EXIT_SUCCESS)
	{
		template = fopen(argv[1],"r");
		output = fopen(temporary_path,"w");

		if(template == NULL || output == NULL)
		{
			perror("configuration files");
			status = EXIT_FAILURE;
		}
	}

	if(status == EXIT_SUCCESS)
	{
		remove(log_path);

		if(compile_check(arguments,&actions,source_path,"int main(void) { return 0; }\n") != 1)
		{
			fprintf(stderr,"Cannot compile and link a program; see %s\n",log_path);
			status = EXIT_FAILURE;
		}
		fputs(signature_line,output);
	}

	char line[512];

	while(status == EXIT_SUCCESS && fgets(line,sizeof(line),template) != NULL)
	{
		char name[128];

		if(sscanf(line,"#cmakedefine01 %127s",name) != 1)
		{
			fputs(line,output);
			continue;
		}
		size_t index = 0;

		while(index < sizeof(checks) / sizeof(checks[0]) && strcmp(name,checks[index].name) != 0)
		{
			++index;
		}

		if(index == sizeof(checks) / sizeof(checks[0]))
		{
			fprintf(stderr,"No compiler probe for %s\n",name);
			status = EXIT_FAILURE;
			break;
		}
		int available = compile_check(arguments,&actions,source_path,checks[index].source);

		if(available < 0)
		{
			status = EXIT_FAILURE;
			break;
		}
		fprintf(output,"#define %s %d\n",name,available);
	}

	if(template != NULL)
	{
		int failed = ferror(template);

		if(fclose(template) != 0)
		{
			failed = 1;
		}

		if(failed != 0)
		{
			perror(argv[1]);
			status = EXIT_FAILURE;
		}
	}

	if(output != NULL)
	{
		int failed = ferror(output);

		if(fclose(output) != 0)
		{
			failed = 1;
		}

		if(failed != 0)
		{
			perror(temporary_path);
			status = EXIT_FAILURE;
		}
	}

	if(status == EXIT_SUCCESS && rename(temporary_path,argv[2]) != 0)
	{
		perror(argv[2]);
		status = EXIT_FAILURE;
	}

	if(status == EXIT_SUCCESS)
	{
		puts("config.h generated with target compiler probes");
	}
	remove(source_path);
	remove(binary_path);
	remove(temporary_path);

	if(actions_initialized == true)
	{
		posix_spawn_file_actions_destroy(&actions);
	}
	free(arguments);
	free(text);
	return status;
}
