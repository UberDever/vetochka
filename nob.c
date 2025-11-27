#include <stdio.h>
#include <string.h>
#define FLAG_IMPLEMENTATION
#include "third_party/flag.h/flag.h"

#define NOB_IMPLEMENTATION
#include "third_party/nob.h/nob.h"

#define ARENA_IMPLEMENTATION
#include "third_party/arena-allocator/arena.h"

#define STB_DS_IMPLEMENTATION
#include "third_party/stb_ds/stb_ds.h"

#define ARENA_SIZE (10 * 1024 * 1024)

#ifdef _WIN32
#define FS_SEP "\\"
#else
#define FS_SEP "/"
#endif

static char g_project_path[1024] = {0};

#define CC     "cc"
#define CFLAGS "--std=c99 -Wall -Wextra"

typedef Arena arena_t;
static arena_t* g_arena;

#define ASSERT_LOG(cond, fmt, ...)                                                                 \
  if (!(cond)) {                                                                                   \
    nob_log(NOB_ERROR, fmt, __VA_ARGS__);                                                          \
    NOB_UNREACHABLE("assertion failed");                                                           \
  }

char* str_concat_v(arena_t* arena, ...) {
  size_t len = 0;
  va_list args;
  va_start(args, arena);
  while (1) {
    const char* arg = va_arg(args, const char*);
    if (arg == NULL) {
      break;
    }
    len += strlen(arg);
  }
  va_end(args);
  char* result = arena_alloc(arena, len + 1);
  if (!result) {
    return NULL;
  }
  result[0] = '\0';

  va_start(args, arena);
  while (1) {
    const char* arg = va_arg(args, const char*);
    if (arg == NULL) {
      break;
    }
    strcat(result, arg);
  }
  va_end(args);
  return result;
}

#define STR_CONCAT(arena, ...) str_concat_v(arena, __VA_ARGS__, NULL)

char* str_alloc(arena_t* arena, const char* str) {
  return STR_CONCAT(arena, str);
}

const char* path_join_v(arena_t* arena, ...) {
  char* result = "";
  va_list args;
  va_start(args, arena);
  const char* arg = va_arg(args, const char*);
  result = STR_CONCAT(arena, arg);
  while (arg != NULL) {
    arg = va_arg(args, const char*);
    if (arg == NULL) {
      break;
    }
    result = STR_CONCAT(arena, result, FS_SEP, arg);
  }
  va_end(args);
  return result;
}

#define PATH_JOIN(arena, ...) path_join_v(arena, __VA_ARGS__, NULL)

typedef struct {
  const char* key;
  char* value;
} build_arg_t;

build_arg_t new_build_arg(arena_t* arena, const char* key, const char* value) {
  const char* k = str_alloc(arena, key);
  char* v = str_alloc(arena, value);
  return (build_arg_t){k, v};
}

typedef struct {
  build_arg_t* items;
  size_t count;
  size_t capacity;
} build_args_t;

char* build_args_get(build_args_t args, const char* key) {
  if (!args.items) {
    return NULL;
  }
  assert(key);
  for (size_t i = 0; i < args.count; ++i) {
    if (0 == strcmp(args.items[i].key, key)) {
      return args.items[i].value;
    }
  }
  return NULL;
}

struct build_target_t;

typedef struct {
  struct build_target_t* items;
  size_t count;
  size_t capacity;
} build_targets_t;

typedef struct {
  const char** items;
  size_t count;
  size_t capacity;
} build_target_inputs_t;

typedef struct build_target_t {
  const char* name;
  build_target_inputs_t ins;
  build_targets_t deps;
  build_args_t build_args;
  bool (*rule)(struct build_target_t);
} build_target_t;

typedef bool (*build_rule_t)(struct build_target_t);

bool build_already_built(build_target_t t) {
  int result = nob_file_exists(t.name);
  return result == 1;
}

bool build_run(build_target_t t) {
  if (build_already_built(t)) {
    return true;
  }
  for (size_t i = 0; i < t.deps.count; ++i) {
    if (!build_run(t.deps.items[i])) {
      return false;
    }
  }
  return t.rule(t);
}

void build_target_free_rec(build_target_t t) {
  for (size_t i = 0; i < t.deps.count; ++i) {
    build_target_free_rec(t.deps.items[i]);
  }
  nob_da_free(t.ins);
  nob_da_free(t.deps);
  nob_da_free(t.build_args);
}

bool build_rule_compile(build_target_t t) {
  Nob_Cmd cmd = {0};
  const char* cc = build_args_get(t.build_args, "cc");
  assert(cc);
  nob_cmd_append(&cmd, cc);

  char* cflags = build_args_get(t.build_args, "cflags");
  assert(cflags);
  for (char* p = strtok(cflags, " "); p != NULL; p = strtok(NULL, " ")) {
    nob_cmd_append(&cmd, p);
  }

  ASSERT_LOG(t.ins.count == 1, "%zu", t.ins.count);
  nob_cmd_append(&cmd, "-c");
  const char* in = t.ins.items[0];
  nob_cmd_append(&cmd, in);

  nob_cmd_append(&cmd, "-o");
  assert(t.name);
  nob_cmd_append(&cmd, t.name);

  bool result = false;
  if (!nob_cmd_run(&cmd)) {
    goto defer;
  }
  result = true;

defer:
  return result;
}

bool build_rule_link_exe(build_target_t t) {
  Nob_Cmd cmd = {0};
  const char* cc = build_args_get(t.build_args, "cc");
  assert(cc);
  nob_cmd_append(&cmd, cc);

  assert(t.ins.items);
  for (size_t i = 0; i < t.ins.count; ++i) {
    assert(t.ins.items[i]);
    nob_cmd_append(&cmd, t.ins.items[i]);
  }

  nob_cmd_append(&cmd, "-o");
  assert(t.name);
  nob_cmd_append(&cmd, t.name);

  bool result = false;
  if (!nob_cmd_run(&cmd)) {
    goto defer;
  }
  result = true;

defer:
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

  bool* help_flag = flag_bool("help", false, "Print the help about a command, or the whole usage");
  char* const* build_cmd = flag_str("build", NULL, "Command; The target we need to build");
  char* const* build_dir_flag = flag_str("build-dir", "build", "Build directory");
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

  if (*help_flag && cmd_activated_num != 0) {
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
  if (argc == 1 || *help_flag) {
    usage(program, stdout);
    exit(0);
  }

  const char* build_dir = build_dir_flag ? *build_dir_flag : NULL;

  char* got_cwd = getcwd(g_project_path, NOB_ARRAY_LEN(g_project_path));
  assert(got_cwd);

  build_target_t t_exe = {0};

  g_arena = arena_create(ARENA_SIZE);
  if (!g_arena) {
    goto errdefer;
  }

  build_target_t t_main = {0};
  {
    t_main.name = PATH_JOIN(g_arena, g_project_path, build_dir, "objs", "main.o");
    nob_da_append(&t_main.build_args, new_build_arg(g_arena, "cc", CC));
    nob_da_append(&t_main.build_args, new_build_arg(g_arena, "cflags", CFLAGS));
    nob_da_append(&t_main.ins, PATH_JOIN(g_arena, g_project_path, "_main.c"));
    t_main.rule = build_rule_compile;
  }
  build_target_t t_lib = {0};
  {
    t_lib.name = PATH_JOIN(g_arena, g_project_path, build_dir, "objs", "lib.o");
    nob_da_append(&t_lib.build_args, new_build_arg(g_arena, "cc", CC));
    nob_da_append(&t_lib.build_args, new_build_arg(g_arena, "cflags", CFLAGS));
    nob_da_append(&t_lib.ins, PATH_JOIN(g_arena, g_project_path, "_lib.c"));
    t_lib.rule = build_rule_compile;
  }

  {
    t_exe.name = PATH_JOIN(g_arena, g_project_path, build_dir, "bin", "a.out");
    nob_da_append(&t_exe.build_args, new_build_arg(g_arena, "cc", CC));
    nob_da_append(&t_exe.ins, t_main.name);
    nob_da_append(&t_exe.ins, t_lib.name);
    nob_da_append(&t_exe.deps, t_main);
    nob_da_append(&t_exe.deps, t_lib);
    t_exe.rule = build_rule_link_exe;
  }

  build_run(t_exe);
  // TODO: clear targets

  goto defer;

errdefer:
defer:
  build_target_free_rec(t_exe);
  arena_destroy(g_arena);
  return 0;
}
