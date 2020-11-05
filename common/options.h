// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <assert.h>
#include <getopt.h> // IWYU pragma: export
#include <stddef.h>

#define MAX_OPTION_ID 10000

typedef enum {
  OPT_GENERIC,
  OPT_NETWORK,
  OPT_RPC,
  OPT_VERBOSITY,
  OPT_ENGINE_CUSTOM,
  OPT_DEPRECATED,
  OPT_ARRAY_END
} option_section_t;

static inline const char *get_option_section_name(option_section_t section) {
  switch (section) {
    case OPT_GENERIC:
      return "Generic";
    case OPT_NETWORK:
      return "Network";
    case OPT_RPC:
      return "RPC";
    case OPT_VERBOSITY:
      return "Verbosity";
    case OPT_ENGINE_CUSTOM:
      return "Custom engine";
    case OPT_DEPRECATED:
      return "Deprecated";
    case OPT_ARRAY_END:
      assert(0);
  }
  assert(0);
  return NULL;
}

typedef int (*options_parser)(int);

void parse_usage();
void remove_all_options();
void init_parse_options(const option_section_t *sections);
int parse_engine_options_long(int argc, char **argv, options_parser execute);
void parse_option(const char *name, int has_arg, int val, const char *help, ...) __attribute__ ((format (printf, 4, 5)));
void parse_common_option(option_section_t section, options_parser parser, const char *name, int has_arg, int val, const char *help, ...) __attribute__ ((format (printf, 6, 7)));
void parse_option_alias(const char *name, char val);
void remove_parse_option(const char *name);
void always_enable_option(const char *name, char *arg);
void usage_and_exit() __attribute__((noreturn));

long long parse_memory_limit(const char *s);
long long parse_memory_limit_default(const char *s, char def_size);
int convert_bytes_num_to_string(long long bytes, char *res, int res_size);
int parse_time_limit(const char *s);

extern void (*usage_options_desc)();
extern void (*usage_other_info)();
void usage_set_other_args_desc(const char *s);


#define COMMON_OPTION_PARSER__(suffix, section, name, letter, has_arg, ...)                                     \
static int OPTION_PARSER_FUNCTION_ ## suffix(int _unused);                                                      \
__attribute__((constructor))                                                                                    \
static void REGISTER_OPTION_PARSER_FUNCTION_ ## suffix() {                                                      \
  parse_common_option((section), OPTION_PARSER_FUNCTION_ ## suffix, (name), (has_arg), (letter), __VA_ARGS__);  \
}                                                                                                               \
static int OPTION_PARSER_FUNCTION_ ## suffix(int _unused __attribute__((unused)))

#define COMMON_OPTION_PARSER_(...) COMMON_OPTION_PARSER__(__VA_ARGS__)
#define OPTION_PARSER(section, name, has_arg, ...) COMMON_OPTION_PARSER_(__COUNTER__, section, name, -1, has_arg, __VA_ARGS__)
#define OPTION_PARSER_SHORT(section, name, letter, has_arg, ...) COMMON_OPTION_PARSER_(__COUNTER__, section, name, letter, has_arg, __VA_ARGS__)

#define FLAG_OPTION_PARSER(section, name, flag, ...)             \
  OPTION_PARSER(section, name, no_argument, __VA_ARGS__) {       \
    flag = 1;                                                    \
    return 0;                                                    \
  }                                                              \

#define SAVE_STRING_OPTION_PARSER(section, name, var, ...)              \
  static int var ## _option_passed = 0;                                 \
  OPTION_PARSER(section, name, required_argument, __VA_ARGS__) {        \
    var = strdup(optarg);                                               \
    var ## _option_passed = 1;                                          \
    return 0;                                                           \
  }                                                                     \
  __attribute__((destructor)) void option_saved_ ## var ## _clear();    \
  void option_saved_ ## var ## _clear() {                               \
    if (var ## _option_passed) free((char *)var);                       \
  }

#define OPTION_PRINT_DEPRECATION_MESSAGE_(option_pattern, ...) \
  kprintf ("warning: option '" option_pattern "' is deprecated and is going to be removed soon, don't use them!\n", __VA_ARGS__);

#define DEPRECATED_OPTION(name, has_arg)              \
  OPTION_PARSER(OPT_DEPRECATED, name, has_arg, " ") { \
    OPTION_PRINT_DEPRECATION_MESSAGE_("--%s", name);  \
    return 0;                                         \
  }

#define DEPRECATED_OPTION_SHORT(name, letter, has_arg)              \
  OPTION_PARSER_SHORT(OPT_DEPRECATED, name, letter, has_arg, " ") { \
    OPTION_PRINT_DEPRECATION_MESSAGE_("--%s/-%c", name, letter);    \
    return 0;                                                       \
  }
