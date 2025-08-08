#include <stdint.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define SRCDIR "./src/"
#define ODIR SRCDIR "obj/"
#define IDIR "./include/"
#define LDIR "./lib/"

void write_args(const char* fname, const char** argv, size_t argc);

/*
#define COMPOUT "compile_commands.json"
#define nob_cc_add_args(cmd, ...) \
  do {\
    nob_cmd_append(cmd, __VA_ARGS__); \
    write_args(COMPOUT, (const char*[]){__VA_ARGS__}, sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*));
  } while (0)
*/

int compile_objs(Cmd *cmd, char **fnames, size_t count);
int compile_lib(Cmd *cmd, char *outname, char **o_fnames, size_t count);
int nob_cc_include(Cmd *cmd);

int main(int argc, char **argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);

  shift(argv, argc);

  if (!mkdir_if_not_exists(LDIR))
    return 1;
  if (!mkdir_if_not_exists(ODIR))
    return 1;

  Cmd cmd = {0};
  char *trgt[] = {"dijkstra", "fheap"};
  const size_t trgt_len = sizeof(trgt) / sizeof(trgt[0]);
  if (!compile_objs(&cmd, trgt, 2))
    return 1;

  // compile fheap
  compile_lib(&cmd, trgt[1], trgt + 1, 1);

  // compile dijkstra
  compile_lib(&cmd, trgt[0], trgt, 2);

  if (argc <= 0) return 0;

  if (strcmp(shift(argv, argc), "main") == 0)
  {
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cc_output(&cmd, "main");
    nob_cc_inputs(&cmd, SRCDIR "main.c");
    nob_cc_flags(&cmd);
    cmd_append(&cmd, "-g");
    nob_cc_include(&cmd);
    cmd_append(&cmd, "-L" LDIR, "-lm");
    for (size_t i = 0; i < trgt_len; ++i)
      cmd_append(&cmd, temp_sprintf("-l%s", trgt[i]));
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    // cmd_append(&cmd, "./main");
    // if (!cmd_run_sync_and_reset(&cmd)) return 1;
  }

  return 0;
}

int nob_cc_include(Cmd *cmd)
{
  cmd_append(cmd, "-I" IDIR, "-I.");
  return 1;
}

int compile_objs(Cmd *cmd, char **fnames, size_t count)
{
  // build every .o file
  for (size_t i = 0; i < count; ++i)
  {
    nob_cc(cmd);
    cmd_append(cmd, "-c");
    nob_cc_output(cmd, temp_sprintf(ODIR "%s.o", fnames[i]));
    nob_cc_inputs(cmd, temp_sprintf(SRCDIR "%s.c", fnames[i]));
    nob_cc_flags(cmd);
    cmd_append(cmd, "-g");
    nob_cc_include(cmd);

    if (!cmd_run_sync_and_reset(cmd))
      return 0;
  }

  return 1;
}

int compile_lib(Cmd *cmd, char *outname, char **o_fnames, size_t count)
{
    cmd_append(cmd, "ar", "rcs");
    cmd_append(cmd, temp_sprintf(LDIR "lib%s.a", outname));
    for (size_t i = 0; i < count; ++i)
      cmd_append(cmd, temp_sprintf(ODIR "%s.o", o_fnames[i]));

    return cmd_run_sync_and_reset(cmd);
}
