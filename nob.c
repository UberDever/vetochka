#include <stdio.h>
#define FLAG_IMPLEMENTATION
#include "third_party/flag.h/flag.h"

#define NOB_IMPLEMENTATION
#include "third_party/nob.h/nob.h"

#define ARENA_IMPLEMENTATION
#include "third_party/arena-allocator/arena.h"

#define STB_DS_IMPLEMENTATION
#include "third_party/stb_ds/stb_ds.h"

typedef Arena arena_t;
static arena_t* g_arena;

typedef struct build_target_t {
  const char* name;
  const char** ins;
  size_t ins_num;
  const struct build_target_t** deps;
  size_t deps_num;
  // TODO: variables map that are tied to arenas
  bool (*rule)(struct build_target_t target);
} build_target_t;

typedef bool (*build_rule_t)(struct build_target_t target);

build_target_t* new_build_target(
    arena_t* arena,
    const char* name,
    const char** ins,
    size_t ins_num,
    const build_target_t** deps,
    size_t deps_num,
    build_rule_t rule) {
  return NULL;
}

bool build_rule_compile() {
  return false;
}

const char* str_concat(arena_t* arena, const char* a, const char* b) {
  size_t lenA = strlen(a);
  size_t lenB = strlen(b);
  char* result = arena_alloc(arena, lenA + lenB + 1);
  if (!result) {
    return NULL;
  }
  memcpy(result, a, lenA);
  memcpy(result + lenA, b, lenB);
  result[lenA + lenB] = '\0';
  return result;
}

void usage(const char* program, FILE* stream) {
  fprintf(stream, "Usage: ./%s [OPTIONS] [--] [ARGS]\n", program);
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  const char* program = argv[0];

  bool* help_cmd = flag_bool("help", false, "Print the help about a command, or the whole usage");
  char* const* build_cmd = flag_str("build", NULL, "Command; The target we need to build");
  char* const* test_cmd = flag_str("test", NULL, "Command; Run specified test");

  if (!flag_parse(argc, argv)) {
    usage(program, stderr);
    flag_print_error(stderr);
    exit(1);
  }
  char* const* cmd_activated_list[] = {
      test_cmd,
      build_cmd,
  };
  size_t cmd_activated_num = 0;
  size_t cmd_activated_i = 0;
  for (size_t i = 0; i < NOB_ARRAY_LEN(cmd_activated_list); ++i) {
    if (*cmd_activated_list[i] != NULL) {
      cmd_activated_num++;
      cmd_activated_i = i;
    }
  }
  if (cmd_activated_num > 1) {
    nob_log(NOB_ERROR, "Only one command can be specified at a time");
    usage(program, stderr);
    exit(1);
  }

  if (*help_cmd && cmd_activated_num != 0) {
    const char* cmd_activated = flag_name((void*)cmd_activated_list[cmd_activated_i]);
    if (0 == strcmp(cmd_activated, "build")) {
      NOB_TODO("help about build");
    } else if (0 == strcmp(cmd_activated, "test")) {
      NOB_TODO("help about test");
    } else {
      char cmd[1024] = {0};
      snprintf(cmd, NOB_ARRAY_LEN(cmd), "unknown command %s", cmd_activated);
      NOB_UNREACHABLE(cmd);
    }
    exit(0);
  }
  if (argc == 1 || *help_cmd) {
    usage(program, stdout);
    exit(0);
  }

  g_arena = arena_create(4096);
  if (!g_arena) {
    goto errdefer;
  }

  //   Nob_Cmd cmd = {0};
  //   nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "main", "main.c");
  //   if (!nob_cmd_run(&cmd))
  //     return 1;

  goto defer;

errdefer:
defer:
  arena_destroy(g_arena);
  return 0;
}
