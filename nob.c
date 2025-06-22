#include <stdint.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define SRCDIR "./src/"
#define ODIR SRCDIR "obj/"
#define IDIR "./include/"
#define LIBDIR "./lib/"

int compile_obj(Cmd *cmd, char **fnames, size_t count);

int main(int argc, char **argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);

  shift(argv, argc);

  if (!mkdir_if_not_exists(LIBDIR))
    return 1;
  if (!mkdir_if_not_exists(ODIR))
    return 1;

  Cmd cmd = {0};
  char *trgt[] = {"dijkstra", "fheap"};
  if (!compile_obj(&cmd, trgt, 2))
    return 1;

  // compile fheap
  cmd_append(&cmd, "ar", "rcs");
  cmd_append(&cmd, LIBDIR "fheap.a");
  cmd_append(&cmd, ODIR "fheap.o");

  if (!cmd_run_sync_and_reset(&cmd))
    return 1;

  // compile dijkstra
  cmd_append(&cmd, "ar", "rcs");
  cmd_append(&cmd, LIBDIR "dijkstra.a");
  cmd_append(&cmd, ODIR "fheap.o", ODIR "dijkstra.o");

  if (!cmd_run_sync_and_reset(&cmd))
    return 1;

  return 0;
}

int compile_obj(Cmd *cmd, char **fnames, size_t count)
{
  // build every .o file
  for (size_t i = 0; i < count; ++i)
  {
    nob_cc(cmd);
    cmd_append(cmd, "-c");
    nob_cc_output(cmd, temp_sprintf(ODIR "%s.o", fnames[i]));
    nob_cc_inputs(cmd, temp_sprintf(SRCDIR "%s.c", fnames[i]));
    nob_cc_flags(cmd);
    cmd_append(cmd, "-I" IDIR);

    if (!cmd_run_sync_and_reset(cmd))
      return 1;
  }

  return 1;
}