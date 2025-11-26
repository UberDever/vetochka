#define NOB_IMPLEMENTATION
#include "third_party/nob.h/nob.h"
#include "third_party/flag.h/flag.h"

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  const char* program = nob_shift(argv, argc);
  (void)program;

  if (argc < 1) {
    nob_log(NOB_ERROR, "Usage");
  }
  const char* cmd = nob_shift(argv, argc);
  //   Nob_Cmd cmd = {0};
  //   nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "main", "main.c");
  //   if (!nob_cmd_run(&cmd))
  //     return 1;
  return 0;
}
