#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/wait.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "./nob.h"

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

  // NOTE: this value is good to my use case
  char temp_key[255];
  char temp_value[2048];

  Nob_String_View sv = nob_sv_from_cstr(buffer);
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

    // TODO: check if the temp_value starts with quote/double quote and report it as it could be a mistake

    setenv(temp_key, temp_value, 1);
  }

  Nob_Cmd cmd = {0};
  nob_da_append_many(&cmd, argv + 2, argc - 2);

  // FIXME: we should not carry about awaiting for the process to finish
  if (!nob_cmd_run_sync(cmd)) return 1;

  // NOTE: the program is about to close, let the OS free the memory itself!
  // free(buffer);

  return 0;
}
