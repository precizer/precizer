#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "launcher_config.h"
#include "launcher_resources.h"

typedef struct {
	const unsigned char *data;
	DWORD size;
} EmbeddedResource;

typedef struct {
	wchar_t *text;
	size_t length;
	size_t capacity;
} WideBuffer;

/**
 * @brief Print the last Windows error with a short context message
 * @param context Operation that failed
 */
static void print_last_error(const wchar_t *context)
{
	DWORD error_code = GetLastError();
	wchar_t *message = NULL;

	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		0,
		(LPWSTR)&message,
		0,
		NULL);

	if(message != NULL)
	{
		fwprintf(stderr,L"%ls failed: %ls",context,message);
		LocalFree(message);

	} else {
		fwprintf(stderr,L"%ls failed with Windows error %lu\n",context,(unsigned long)error_code);
	}
}

/**
 * @brief Initialize a wide-character buffer
 * @param buffer Buffer descriptor to initialize
 */
static void wide_buffer_init(WideBuffer *buffer)
{
	buffer->text = NULL;
	buffer->length = 0U;
	buffer->capacity = 0U;
}

/**
 * @brief Free memory owned by a wide-character buffer
 * @param buffer Buffer descriptor to release
 */
static void wide_buffer_free(WideBuffer *buffer)
{
	free(buffer->text);
	buffer->text = NULL;
	buffer->length = 0U;
	buffer->capacity = 0U;
}

/**
 * @brief Reserve enough buffer space for a requested character count
 * @param buffer Buffer descriptor to grow
 * @param required Required number of wide characters, including the terminator
 * @return true when the buffer has enough space
 */
static bool wide_buffer_reserve(WideBuffer *buffer,size_t required)
{
	bool success = true;

	if(required > buffer->capacity)
	{
		size_t new_capacity = buffer->capacity;

		if(new_capacity == 0U)
		{
			new_capacity = 128U;
		}

		while(success == true && new_capacity < required)
		{
			if(new_capacity > (SIZE_MAX / 2U))
			{
				success = false;

			} else {
				new_capacity *= 2U;
			}
		}

		if(success == true)
		{
			wchar_t *new_text = realloc(buffer->text,new_capacity * sizeof(wchar_t));

			if(new_text == NULL)
			{
				success = false;

			} else {
				buffer->text = new_text;
				buffer->capacity = new_capacity;
			}
		}
	}

	return(success);
}

/**
 * @brief Append one wide character to a buffer
 * @param buffer Buffer descriptor to update
 * @param character Character to append
 * @return true when the character was appended
 */
static bool wide_buffer_append_char(WideBuffer *buffer,wchar_t character)
{
	bool success = wide_buffer_reserve(buffer,buffer->length + 2U);

	if(success == true)
	{
		buffer->text[buffer->length] = character;
		buffer->length++;
		buffer->text[buffer->length] = L'\0';
	}

	return(success);
}

/**
 * @brief Append a wide string to a buffer
 * @param buffer Buffer descriptor to update
 * @param text Text to append
 * @return true when the text was appended
 */
static bool wide_buffer_append(WideBuffer *buffer,const wchar_t *text)
{
	bool success = true;

	if(text == NULL)
	{
		success = false;
	}

	if(success == true)
	{
		size_t text_length = wcslen(text);
		success = wide_buffer_reserve(buffer,buffer->length + text_length + 1U);

		if(success == true)
		{
			memcpy(buffer->text + buffer->length,text,(text_length + 1U) * sizeof(wchar_t));
			buffer->length += text_length;
		}
	}

	return(success);
}

/**
 * @brief Append a single path component with a separating backslash
 * @param path Path buffer to update
 * @param component Path component to append
 * @return true when the component was appended
 */
static bool wide_buffer_append_path_component(WideBuffer *path,const wchar_t *component)
{
	bool success = true;

	if(path->length > 0U)
	{
		wchar_t last = path->text[path->length - 1U];

		if(last != L'\\' && last != L'/')
		{
			success = wide_buffer_append_char(path,L'\\');
		}
	}

	if(success == true)
	{
		success = wide_buffer_append(path,component);
	}

	return(success);
}

/**
 * @brief Read an environment variable into a wide-character buffer
 * @param name Environment variable name
 * @param output Destination buffer
 * @return true when the value was read
 */
static bool get_environment_value(const wchar_t *name,WideBuffer *output)
{
	bool success = true;
	DWORD required = GetEnvironmentVariableW(name,NULL,0);

	if(required == 0)
	{
		print_last_error(L"GetEnvironmentVariableW");
		success = false;
	}

	if(success == true)
	{
		success = wide_buffer_reserve(output,(size_t)required);
	}

	if(success == true)
	{
		DWORD written = GetEnvironmentVariableW(name,output->text,required);

		if(written == 0 || written >= required)
		{
			print_last_error(L"GetEnvironmentVariableW");
			success = false;

		} else {
			output->length = (size_t)written;
		}
	}

	return(success);
}

/**
 * @brief Create a directory or accept an already existing one
 * @param path Directory path
 * @return true when the directory exists after the call
 */
static bool create_directory_if_missing(const wchar_t *path)
{
	bool success = true;

	if(CreateDirectoryW(path,NULL) == 0)
	{
		DWORD error_code = GetLastError();

		if(error_code != ERROR_ALREADY_EXISTS)
		{
			print_last_error(L"CreateDirectoryW");
			success = false;
		}
	}

	return(success);
}

/**
 * @brief Build and create the payload cache directory
 * @param output Receives the final cache directory path
 * @return true when the cache directory exists
 */
static bool prepare_cache_directory(WideBuffer *output)
{
	bool success = get_environment_value(L"LOCALAPPDATA",output);

	if(success == true)
	{
		success = wide_buffer_append_path_component(output,L"Precizer");
	}

	if(success == true)
	{
		success = create_directory_if_missing(output->text);
	}

	if(success == true)
	{
		success = wide_buffer_append_path_component(output,L"precizer");
	}

	if(success == true)
	{
		success = create_directory_if_missing(output->text);
	}

	if(success == true)
	{
		success = wide_buffer_append_path_component(output,PRECIZER_LAUNCHER_VERSION);
	}

	if(success == true)
	{
		success = create_directory_if_missing(output->text);
	}

	if(success == true)
	{
		success = wide_buffer_append_path_component(output,PRECIZER_LAUNCHER_PAYLOAD_HASH);
	}

	if(success == true)
	{
		success = create_directory_if_missing(output->text);
	}

	return(success);
}

/**
 * @brief Load an embedded resource from the current executable
 * @param resource_id Numeric resource identifier
 * @param output Receives the resource bytes and size
 * @return true when the resource was found and locked
 */
static bool load_embedded_resource(int resource_id,EmbeddedResource *output)
{
	bool success = true;
	HMODULE module = GetModuleHandleW(NULL);
	HRSRC resource = FindResourceW(module,MAKEINTRESOURCEW(resource_id),RT_RCDATA);

	if(resource == NULL)
	{
		print_last_error(L"FindResourceW");
		success = false;
	}

	HGLOBAL loaded_resource = NULL;

	if(success == true)
	{
		loaded_resource = LoadResource(module,resource);

		if(loaded_resource == NULL)
		{
			print_last_error(L"LoadResource");
			success = false;
		}
	}

	if(success == true)
	{
		output->data = LockResource(loaded_resource);
		output->size = SizeofResource(module,resource);

		if(output->data == NULL || output->size == 0U)
		{
			fwprintf(stderr,L"Embedded resource %d is empty or unavailable\n",resource_id);
			success = false;
		}
	}

	return(success);
}

/**
 * @brief Compare an existing file with embedded resource bytes
 * @param path File path to inspect
 * @param resource Embedded resource to compare
 * @return true when the file exists and has identical contents
 */
static bool file_matches_resource(const wchar_t *path,const EmbeddedResource *resource)
{
	bool matches = false;
	HANDLE file = CreateFileW(path,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(file != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;

		if(GetFileSizeEx(file,&file_size) != 0 && file_size.QuadPart == (LONGLONG)resource->size)
		{
			unsigned char buffer[65536];
			const unsigned char *resource_cursor = resource->data;
			DWORD remaining = resource->size;
			matches = true;

			while(matches == true && remaining > 0U)
			{
				DWORD chunk_size = remaining;

				if(chunk_size > (DWORD)sizeof(buffer))
				{
					chunk_size = (DWORD)sizeof(buffer);
				}

				DWORD bytes_read = 0;

				if(ReadFile(file,buffer,chunk_size,&bytes_read,NULL) == 0 || bytes_read != chunk_size)
				{
					matches = false;

				} else if(memcmp(buffer,resource_cursor,chunk_size) != 0){
					matches = false;

				} else {
					resource_cursor += chunk_size;
					remaining -= chunk_size;
				}
			}
		}

		CloseHandle(file);
	}

	return(matches);
}

/**
 * @brief Write embedded resource bytes to a file through a temporary path
 * @param path Destination path
 * @param resource Embedded resource to write
 * @return true when the destination file contains the resource bytes
 */
static bool write_resource_file(const wchar_t *path,const EmbeddedResource *resource)
{
	bool success = true;
	WideBuffer temporary_path;
	wide_buffer_init(&temporary_path);

	success = wide_buffer_append(&temporary_path,path);

	if(success == true)
	{
		wchar_t suffix[64];
		int suffix_length = swprintf(suffix,sizeof(suffix) / sizeof(suffix[0]),L".%lu.tmp",(unsigned long)GetCurrentProcessId());

		if(suffix_length < 0)
		{
			success = false;

		} else {
			success = wide_buffer_append(&temporary_path,suffix);
		}
	}

	HANDLE file = INVALID_HANDLE_VALUE;

	if(success == true)
	{
		file = CreateFileW(temporary_path.text,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

		if(file == INVALID_HANDLE_VALUE)
		{
			print_last_error(L"CreateFileW");
			success = false;
		}
	}

	if(success == true)
	{
		const unsigned char *cursor = resource->data;
		DWORD remaining = resource->size;

		while(success == true && remaining > 0U)
		{
			DWORD chunk_size = remaining;

			if(chunk_size > 65536U)
			{
				chunk_size = 65536U;
			}

			DWORD bytes_written = 0;

			if(WriteFile(file,cursor,chunk_size,&bytes_written,NULL) == 0 || bytes_written != chunk_size)
			{
				print_last_error(L"WriteFile");
				success = false;

			} else {
				cursor += chunk_size;
				remaining -= chunk_size;
			}
		}
	}

	if(file != INVALID_HANDLE_VALUE)
	{
		if(FlushFileBuffers(file) == 0)
		{
			print_last_error(L"FlushFileBuffers");
			success = false;
		}

		if(CloseHandle(file) == 0)
		{
			print_last_error(L"CloseHandle");
			success = false;
		}
	}

	if(success == true)
	{
		if(MoveFileExW(temporary_path.text,path,MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0)
		{
			print_last_error(L"MoveFileExW");
			success = false;
		}
	}

	if(success != true && temporary_path.text != NULL)
	{
		DeleteFileW(temporary_path.text);
	}

	wide_buffer_free(&temporary_path);

	return(success);
}

/**
 * @brief Ensure a cached file matches an embedded resource
 * @param path Cached file path
 * @param resource Embedded resource that should be present
 * @return true when the cached file is ready
 */
static bool ensure_embedded_file(const wchar_t *path,const EmbeddedResource *resource)
{
	bool success = true;

	if(file_matches_resource(path,resource) == false)
	{
		success = write_resource_file(path,resource);
	}

	return(success);
}

/**
 * @brief Append a quoted Windows command-line argument
 * @param command_line Command-line buffer to update
 * @param argument Argument text to quote
 * @return true when the quoted argument was appended
 */
static bool append_quoted_argument(WideBuffer *command_line,const wchar_t *argument)
{
	bool success = wide_buffer_append_char(command_line,L'"');
	size_t backslashes = 0U;

	for(size_t index = 0U; success == true && argument[index] != L'\0'; index++)
	{
		wchar_t character = argument[index];

		if(character == L'\\')
		{
			backslashes++;

		} else {
			size_t slash_count = backslashes;

			if(character == L'"')
			{
				slash_count = (backslashes * 2U) + 1U;
			}

			for(size_t slash_index = 0U; success == true && slash_index < slash_count; slash_index++)
			{
				success = wide_buffer_append_char(command_line,L'\\');
			}

			backslashes = 0U;

			if(success == true)
			{
				success = wide_buffer_append_char(command_line,character);
			}
		}
	}

	for(size_t slash_index = 0U; success == true && slash_index < (backslashes * 2U); slash_index++)
	{
		success = wide_buffer_append_char(command_line,L'\\');
	}

	if(success == true)
	{
		success = wide_buffer_append_char(command_line,L'"');
	}

	return(success);
}

/**
 * @brief Build the child command line from wrapper arguments
 * @param payload_path Extracted MSYS payload executable path
 * @param argument_count Number of wrapper arguments
 * @param arguments Wrapper arguments
 * @param output Receives the child command line
 * @return true when the command line was built
 */
static bool build_child_command_line(
	const wchar_t *payload_path,
	int           argument_count,
	wchar_t       **arguments,
	WideBuffer    *output)
{
	bool success = append_quoted_argument(output,payload_path);

	for(int index = 1; success == true && index < argument_count; index++)
	{
		success = wide_buffer_append_char(output,L' ');

		if(success == true)
		{
			success = append_quoted_argument(output,arguments[index]);
		}
	}

	return(success);
}

/**
 * @brief Run the extracted payload and return its exit code
 * @param command_line Mutable child command line
 * @param exit_code Receives the child process exit code
 * @return true when the child process was started and waited for
 */
static bool run_payload(wchar_t *command_line,DWORD *exit_code)
{
	bool success = true;
	STARTUPINFOW startup_info;
	PROCESS_INFORMATION process_info;

	memset(&startup_info,0,sizeof(startup_info));
	memset(&process_info,0,sizeof(process_info));
	startup_info.cb = sizeof(startup_info);

	if(CreateProcessW(NULL,command_line,NULL,NULL,TRUE,0,NULL,NULL,&startup_info,&process_info) == 0)
	{
		print_last_error(L"CreateProcessW");
		success = false;
	}

	if(success == true)
	{
		DWORD wait_status = WaitForSingleObject(process_info.hProcess,INFINITE);

		if(wait_status != WAIT_OBJECT_0)
		{
			print_last_error(L"WaitForSingleObject");
			success = false;
		}
	}

	if(success == true)
	{
		if(GetExitCodeProcess(process_info.hProcess,exit_code) == 0)
		{
			print_last_error(L"GetExitCodeProcess");
			success = false;
		}
	}

	if(process_info.hThread != NULL)
	{
		CloseHandle(process_info.hThread);
	}

	if(process_info.hProcess != NULL)
	{
		CloseHandle(process_info.hProcess);
	}

	return(success);
}

/**
 * @brief Launcher entry point
 * @return Payload exit code on success, or 1 when launcher setup fails
 */
int main(void)
{
	int exit_code = 1;
	bool success = true;
	int argument_count = 0;
	wchar_t **arguments = CommandLineToArgvW(GetCommandLineW(),&argument_count);
	EmbeddedResource precizer_resource = {0};
	EmbeddedResource msys_resource = {0};
	WideBuffer cache_directory;
	WideBuffer payload_path;
	WideBuffer runtime_path;
	WideBuffer command_line;

	wide_buffer_init(&cache_directory);
	wide_buffer_init(&payload_path);
	wide_buffer_init(&runtime_path);
	wide_buffer_init(&command_line);

	if(arguments == NULL)
	{
		print_last_error(L"CommandLineToArgvW");
		success = false;
	}

	if(success == true)
	{
		success = load_embedded_resource(IDR_PRECIZER_EXE,&precizer_resource);
	}

	if(success == true)
	{
		success = load_embedded_resource(IDR_MSYS_DLL,&msys_resource);
	}

	if(success == true)
	{
		success = prepare_cache_directory(&cache_directory);
	}

	if(success == true)
	{
		success = wide_buffer_append(&payload_path,cache_directory.text);
	}

	if(success == true)
	{
		success = wide_buffer_append_path_component(&payload_path,L"precizer.exe");
	}

	if(success == true)
	{
		success = wide_buffer_append(&runtime_path,cache_directory.text);
	}

	if(success == true)
	{
		success = wide_buffer_append_path_component(&runtime_path,L"msys-2.0.dll");
	}

	if(success == true)
	{
		success = ensure_embedded_file(payload_path.text,&precizer_resource);
	}

	if(success == true)
	{
		success = ensure_embedded_file(runtime_path.text,&msys_resource);
	}

	if(success == true)
	{
		success = build_child_command_line(payload_path.text,argument_count,arguments,&command_line);
	}

	if(success == true)
	{
		DWORD payload_exit_code = 1U;
		success = run_payload(command_line.text,&payload_exit_code);

		if(success == true)
		{
			exit_code = (int)payload_exit_code;
		}
	}

	if(arguments != NULL)
	{
		LocalFree(arguments);
	}

	wide_buffer_free(&cache_directory);
	wide_buffer_free(&payload_path);
	wide_buffer_free(&runtime_path);
	wide_buffer_free(&command_line);

	return(exit_code);
}
