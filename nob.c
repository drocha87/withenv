#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "./nob.h"

int main(int argc, char **argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);
  const char *program = shift_args(&argc, &argv);
  UNUSED(program);
  Cmd cmd = {0};
  cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-Wunused", "-pedantic", "-o", "withenv", "main.c");
  return cmd_run_sync(cmd) ? 0 : 1;
}
