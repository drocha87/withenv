#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/wait.h>
#include <unistd.h>

// nob.h HEADER BEGIN ////////////////////////////////////////
// The next code was stolen from https://github.com/tsoding/nobuild

typedef struct {
	size_t count;
	const char *data;
} Nob_String_View;

Nob_String_View nob_sv_from_parts(const char *data, size_t count)
{
	Nob_String_View sv;
	sv.count = count;
	sv.data = data;
	return sv;
}

Nob_String_View nob_sv_chop_by_delim(Nob_String_View *sv, char delim)
{
	size_t i = 0;
	while (i < sv->count && sv->data[i] != delim) {
		i += 1;
	}

	Nob_String_View result = nob_sv_from_parts(sv->data, i);

	if (i < sv->count) {
		sv->count -= i + 1;
		sv->data  += i + 1;
	} else {
		sv->count -= i;
		sv->data  += i;
	}

	return result;
}

Nob_String_View nob_sv_trim_left(Nob_String_View sv)
{
	size_t i = 0;
	while (i < sv.count && isspace(sv.data[i])) {
		i += 1;
	}

	return nob_sv_from_parts(sv.data + i, sv.count - i);
}

Nob_String_View nob_sv_trim_right(Nob_String_View sv)
{
	size_t i = 0;
	while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
		i += 1;
	}

	return nob_sv_from_parts(sv.data, sv.count - i);
}

Nob_String_View nob_sv_trim(Nob_String_View sv)
{
	return nob_sv_trim_right(nob_sv_trim_left(sv));
}

Nob_String_View nob_sv_from_cstr(const char *cstr)
{
	return nob_sv_from_parts(cstr, strlen(cstr));
}

// Initial capacity of a dynamic array
#define NOB_DA_INIT_CAP 256

typedef struct {
		char *items;
		size_t count;
		size_t capacity;
} Nob_String_Builder;

// Append an item to a dynamic array
#define nob_da_append(da, item)                                                         \
		do {                                                                                \
				if ((da)->count >= (da)->capacity) {                                            \
						(da)->capacity = (da)->capacity == 0 ? NOB_DA_INIT_CAP : (da)->capacity*2;  \
						(da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items));		\
						assert((da)->items != NULL && "Buy more RAM lol");													\
				}                                                                               \
																																												\
				(da)->items[(da)->count++] = (item);                                            \
		} while (0)

// Append several items to a dynamic array
#define nob_da_append_many(da, new_items, new_items_count)																			\
		do {																																												\
				if ((da)->count + (new_items_count) > (da)->capacity) {																	\
						if ((da)->capacity == 0) {																													\
								(da)->capacity = NOB_DA_INIT_CAP;																								\
						}																																										\
						while ((da)->count + (new_items_count) > (da)->capacity) {													\
								(da)->capacity *= 2;																														\
						}																																										\
						(da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items));						\
						assert((da)->items != NULL && "Buy more RAM lol");																	\
				}																																												\
				memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
				(da)->count += (new_items_count);																												\
		} while (0)

// Append a NULL-terminated string to a string builder
#define nob_sb_append_cstr(sb, cstr)  \
		do {                              \
				const char *s = (cstr);       \
				size_t n = strlen(s);         \
				nob_da_append_many(sb, s, n); \
		} while (0)

// Append a single NULL character at the end of a string builder. So then you can
// use it a NULL-terminated C string
#define nob_sb_append_null(sb) nob_da_append_many(sb, "", 1)

#define nob_da_free(da) free((da).items)

#define NOB_INVALID_PROC (-1)

typedef int Nob_Proc;

// Wait until the process has finished
bool nob_proc_wait(Nob_Proc proc)
{
	if (proc == NOB_INVALID_PROC) return false;
	for (;;) {
		int wstatus = 0;
		if (waitpid(proc, &wstatus, 0) < 0) {
			fprintf(stderr, "could not wait on command (pid %d): %s", proc, strerror(errno));
			return false;
		}

		if (WIFEXITED(wstatus)) {
			int exit_status = WEXITSTATUS(wstatus);
			if (exit_status != 0) {
				fprintf(stderr, "command exited with exit code %d", exit_status);
				return false;
			}

			break;
		}

		if (WIFSIGNALED(wstatus)) {
			fprintf(stderr, "command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
			return false;
		}
	}
	return true;
}

// A command - the main workhorse of Nob. Nob is all about building commands an running them
typedef struct {
	const char **items;
	size_t count;
	size_t capacity;
} Nob_Cmd;

#define nob_cmd_append(cmd, ...)												\
		nob_da_append_many(cmd,															\
											 ((const char*[]){__VA_ARGS__}),	\
											 (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

// Free all the memory allocated by command arguments
#define nob_cmd_free(cmd) free(cmd.items)

Nob_Proc nob_cmd_run_async(Nob_Cmd cmd, char *env_buffer)
{
	if (cmd.count < 1) {
		fprintf(stderr, "Could not run empty command");
		return NOB_INVALID_PROC;
	}

	pid_t cpid = fork();
	if (cpid < 0) {
		fprintf(stderr, "Could not fork child process: %s", strerror(errno));
		return NOB_INVALID_PROC;
	}

	if (cpid == 0) {
		// NOTE: this value is good to my use case
		char temp_key[255];
		char temp_value[2048];

		Nob_String_View sv = nob_sv_from_cstr(env_buffer);
		while (sv.count > 0) {
			Nob_String_View line = nob_sv_trim(nob_sv_chop_by_delim(&sv, '\n'));
			if (line.count <= 0 || line.data[0] == '#') {
				// NOTE: if line is empty or starts with a # just ignore it
				continue;
			}

			// FIXME: we don't handle errors or special cases like if the value is wrapped in "
			Nob_String_View key   = nob_sv_trim(nob_sv_chop_by_delim(&line, '='));
			Nob_String_View value = nob_sv_trim(line);

			memcpy(temp_key, key.data, key.count);
			temp_key[key.count] = '\0';

			memcpy(temp_value, value.data, value.count);
			temp_value[value.count] = '\0';

			setenv(temp_key, temp_value, 1);
		}

		// NOTE: This leaks a bit of memory in the child process.
		// But do we actually care? It's a one off leak anyway...
		Nob_Cmd cmd_null = {0};
		nob_da_append_many(&cmd_null, cmd.items, cmd.count);
		nob_cmd_append(&cmd_null, NULL);

		if (execvp(cmd.items[0], (char * const*) cmd_null.items) < 0) {
			fprintf(stderr, "Could not exec child process: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		assert(0 && "unreachable");
	}

	return cpid;
}

// Run command synchronously
bool nob_cmd_run_sync(Nob_Cmd cmd, char * env_buffer)
{
	Nob_Proc p = nob_cmd_run_async(cmd, env_buffer);
	if (p == NOB_INVALID_PROC) return false;
	return nob_proc_wait(p);
}

// nob.h HEADER END ////////////////////////////////////////

void usage(void) {
		printf("Usage: withenv env_file command [args...]\n");
		printf("\n");
		printf("  env_file   Path to the environment file containing key=value pairs\n");
		printf("  command    The command to be executed with the specified environment variables\n");
		printf("  args...    Arguments to be passed to the command\n");
		printf("\n");
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		usage();
		exit(EXIT_FAILURE);
	}

	char *pathname = argv[1];
	FILE *env_file = fopen(pathname, "rb");

	if (fseek(env_file, 0, SEEK_END) < 0) {
		perror("main");
		exit (EXIT_FAILURE);
	}

	long size = ftell(env_file);
	if (size < 0) {
		perror("main");
		exit (EXIT_FAILURE);
	}

	if (fseek(env_file, 0, SEEK_SET) < 0) {
		perror("main");
		exit (EXIT_FAILURE);
	}

	char *buffer = (char*)malloc(size);
	if (buffer == NULL) {
		perror("main");
		exit (EXIT_FAILURE);
	}

	fread(buffer, size, 1, env_file);
	if (ferror(env_file)) {
		perror("main");
		exit (EXIT_FAILURE);
	}

	if (fclose(env_file) != 0) {
		perror("main");
		exit(EXIT_FAILURE);
	}

	Nob_Cmd cmd = {0};
	nob_da_append_many(&cmd, argv + 2, argc - 2);

	// FIXME: we should not carry about awaiting for the process to finish
	if (!nob_cmd_run_sync(cmd, buffer)) return 1;

	// NOTE: the program is about to close, let the OS free the memory itself!
	// free(buffer);

	return 0;
}
