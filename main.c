#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "./nob.h"

void usage(const char *program) {
    printf("Usage: %s env_file command [args...]\n", program);
    printf("\n");
    printf("  env_file   Path to the environment file containing key=value pairs\n");
    printf("  command    The command to be executed with the specified environment variables\n");
    printf("  args...    Arguments to be passed to the command\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    const char *program = shift(argv, argc);
    if (argc <= 0) {
        usage(program);
        return 1;
    }

    const char *env_file_path = shift(argv, argc);
    if (argc <= 0) {
        usage(program);
        return 1;
    }

    load_env_file(env_file_path, 1);

    Cmd cmd = {0};
    da_append_many(&cmd, argv, argc);

    if (!cmd_run_sync(cmd)) return 1;

    return 0;
}
