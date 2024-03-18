// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/options.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <vector>

#include "common/algorithms/find.h"
#include "common/kprintf.h"
#include "common/smart_ptrs/singleton.h"
#include "common/version-string.h"

OPTION_PARSER_SHORT(OPT_GENERIC, "help", 'h', no_argument, "prints help and exits") {
  usage_and_exit();
}

struct engine_option_t {
  std::string name;
  char short_name = 0;
  int has_arg = 0;
  int alias_to = 0;
  std::string help;
  options_parser parser = nullptr;
  option_section_t section = OPT_ARRAY_END;
};

static vk::singleton<std::array<engine_option_t, MAX_OPTION_ID>> options;
static bool need_rebuild = true;

int is_valid_option(int val) {
  return !options[val].name.empty();
}

int get_option_alias(int val) {
  return options[val].alias_to;
}

static std::string format_help_arg_name(int val) {
  std::stringstream result;
  result << "\t";
  result << "--" << options[val].name;
  if (options[val].short_name) {
    result << "/-" << options[val].short_name;
  }
  if (options[val].has_arg == required_argument) {
    result << " <arg>";
  } else if (options[val].has_arg == optional_argument) {
    result << "[={arg}]";
  }
  return result.str();
}

void parse_usage() {
  int max_len = 0;
  std::vector<option_section_t> sections;
  for (int i = 1; i < MAX_OPTION_ID; i++) {
    if (is_valid_option(i)) {
      auto name_str = format_help_arg_name(i);
      if (name_str.size() > max_len) {
        max_len = name_str.size();
      }
      sections.push_back(options[i].section);
    }
  }
  std::sort(sections.begin(), sections.end());
  sections.erase(std::unique(sections.begin(), sections.end()), sections.end());
  for (const auto &section : sections) {
    printf("\n%s options:\n", get_option_section_name(section));
    for (int i = 1; i < MAX_OPTION_ID; i++) {
      if (is_valid_option(i) && options[i].section == section) {
        auto name_str = format_help_arg_name(i);
        printf("%s", name_str.c_str());
        int cur = name_str.size();
        while (cur < max_len) {
          printf(" ");
          cur++;
        }
        printf("\t");
        assert(!options[i].help.empty());
        for (char c : options[i].help) {
          printf("%c", c);
          if (c == '\n') {
            printf("\t");
            int k;
            for (k = 0; k < max_len; k++) {
              printf(" ");
            }
            printf("\t");
          }
        }
        printf("\n");
      }
    }
  }
}

static void vparse_common_option(option_section_t section, options_parser parser, const char *name, int has_arg, int val, const char *help, va_list ap) {
  need_rebuild = true;
  if (vk::none_of_equal(has_arg, no_argument, required_argument, optional_argument)) {
    kprintf("Strange has_arg value %d\n", has_arg);
    exit(1);
  }
  if (val <= 0 || val >= MAX_OPTION_ID) {
    kprintf("Invalid option id %d\n", val);
    exit(1);
  }
  if (is_valid_option(val) || get_option_alias(val)) {
    if (33 <= val && val <= 127) {
      kprintf("Duplicate option `%c`\n", (char)val);
    } else {
      kprintf("Duplicate option %d\n", val);
    }
    exit(5);
  }

  assert(name);
  assert(help);
  options[val].name = name;
  options[val].has_arg = has_arg;
  options[val].parser = parser;
  options[val].section = section;
  static char help_buffer[65536];
  assert(vsnprintf(help_buffer, sizeof(help_buffer), help, ap) >= 0);
  options[val].help = help_buffer;
  if (33 <= val && val <= 127) {
    options[val].short_name = (char)val;
  }
}

void parse_option(const char *name, int has_arg, int val, const char *help, ...) {
  va_list ap;
  va_start(ap, help);
  vparse_common_option(OPT_ENGINE_CUSTOM, nullptr, name, has_arg, val, help, ap);
  va_end(ap);
}

void parse_common_option(option_section_t section, options_parser parser, const char *name, int has_arg, int val, const char *help, ...) {
  static int useless_option_id = 5000;
  if (val == -1) {
    val = useless_option_id++;
  }
  va_list ap;
  va_start(ap, help);
  vparse_common_option(section, parser, name, has_arg, val, help, ap);
  va_end(ap);
}

static int find_parse_option_by_name(const char *name) {
  int i;
  for (i = 0; i < MAX_OPTION_ID; i++) {
    while (options[i].name == name) {
      return i;
    }
  }
  return 0;
}

void parse_option_alias(const char *name, char val) {
  need_rebuild = true;
  assert(name);
  assert(val >= 33 && static_cast<unsigned char>(val) <= 127);
  int opt_id = find_parse_option_by_name(name);
  if (!opt_id) {
    kprintf("Try to alias unexistent option %s\n", name);
    exit(1);
  }
  if (options[opt_id].short_name) {
    kprintf("Option %s already have short name %c\n", name, options[opt_id].short_name);
    exit(1);
  }
  if (is_valid_option(val) || get_option_alias(val)) {
    kprintf("Can't alias %c to %s, it already used as option\n", val, name);
    exit(1);
  }
  options[opt_id].short_name = val;
  options[(int)val].alias_to = opt_id;
}

static void remove_option_by_index(int val) {
  need_rebuild = true;
  options[val] = engine_option_t();
}

void remove_parse_option(const char *name) {
  int val = find_parse_option_by_name(name);
  if (!val) {
    send_message_to_assertion_chat("Try to remove unexistent option %s\n", name);
    return;
  }
  assert(is_valid_option(val));
  remove_option_by_index(val);
}

void remove_all_options() {
  for (int i = 0; i < MAX_OPTION_ID; i++) {
    remove_option_by_index(i);
  }
}

void init_parse_options(const option_section_t *sections) {
  for (int i = 0; i < MAX_OPTION_ID; i++) {
    if (is_valid_option(i)) {
      int to_save = 0;
      for (int j = 0; sections[j] != OPT_ARRAY_END; j++) {
        if (options[i].section == sections[j]) {
          to_save = 1;
        }
      }
      if (!to_save) {
        remove_option_by_index(i);
      }
    }
  }
}

long long parse_memory_limit_default(const char *s, char def_size) {
  long long x;
  char c = 0;
  if (sscanf(s, "%lld%c", &x, &c) < 1) {
    kprintf("Parsing memory limit for option fail: %s\n", s);
    exit(1);
  }
  if (c == 0) {
    if (def_size) {
      kprintf("No size modifier given in parse_memory_limit, using default (%c)\n", def_size);
    }
    c = def_size;
  }
  switch (c | 0x20) {
    case ' ':
      break;
    case 'b':
      break;
    case 'k':
      x <<= 10;
      break;
    case 'm':
      x <<= 20;
      break;
    case 'g':
      x <<= 30;
      break;
    case 't':
      x <<= 40;
      break;
    default:
      kprintf("Parsing memory limit fail. Unknown suffix '%c'.\n", c);
      exit(1);
  }
  return x;
}

int convert_bytes_num_to_string(long long bytes, char *res, int res_size) {
  const char suf[] = {' ', 'k', 'm', 'g', 't'};
  constexpr int suf_count = sizeof(suf);

  int cur_suf = 0;
  double result = bytes;
  while (result > 1e3 && cur_suf + 1 < suf_count) {
    result /= 1024;
    cur_suf++;
  }
  return snprintf(res, res_size, "%.3f%c", result, suf[cur_suf]);
}

int parse_time_limit(const char *s) {
  int x = 0;
  char c = 0;
  if (sscanf(s, "%d%c", &x, &c) < 1) {
    kprintf("Parsing time limit for option fail: %s\n", s);
    exit(1);
  }
  switch (c | 0x20) {
    case ' ':
      break;
    case 's':
      break;
    case 'm':
      x *= 60;
      break;
    case 'h':
      x *= 3600;
      break;
    case 'd':
      x *= 86400;
      break;
    default:
      kprintf("Parsing time limit fail. Unknown suffix '%c'.\n", c);
      exit(1);
  }
  return x;
}

long long parse_memory_limit(const char *s) {
  return parse_memory_limit_default(s, 0);
}

static const char *usage_other_args_desc = "<kfs-binlog-prefix>";
static const char *usage_progname;
void (*usage_options_desc)() = parse_usage;
void (*usage_other_info)() = nullptr;

void usage_set_other_args_desc(const char *s) {
  usage_other_args_desc = s;
}

void usage_and_exit() {
  printf("%s\n", get_version_string());
  printf("usage: %s %s\n", usage_progname ?: "", usage_other_args_desc);
  if (usage_options_desc) {
    usage_options_desc();
  }
  if (usage_other_info) {
    usage_other_info();
  }
  exit(2);
}

int parse_engine_options_long(int argc, char **argv, options_parser execute) {
  usage_progname = argv[0];
  std::string opt_string;
  std::vector<option> long_opts;

  constexpr int PRINT_OPT_ARGS = MAX_OPTION_ID + 1;
  constexpr int PRINT_OPT_NO_ARGS = MAX_OPTION_ID + 2;

  auto rebuild_options = [&]() {
    long_opts.clear();
    opt_string.clear();
    auto add_option = [&](int has_arg, const char *name, int val) {
      long_opts.emplace_back();
      long_opts.back().has_arg = has_arg;
      long_opts.back().name = name;
      long_opts.back().val = val;
      long_opts.back().flag = nullptr;
    };
    for (int i = 1; i < MAX_OPTION_ID; i++) {
      if (!is_valid_option(i)) {
        continue;
      }
      if (options[i].short_name) {
        opt_string += options[i].short_name;
        if (options[i].has_arg == required_argument) {
          opt_string += ':';
        }
      }
      add_option(options[i].has_arg, options[i].name.c_str(), i);
    }
    add_option(no_argument, "print-engine-options-args", PRINT_OPT_ARGS);
    add_option(no_argument, "print-engine-options-no-args", PRINT_OPT_NO_ARGS);
    long_opts.emplace_back();
  };

  rebuild_options();

  auto print_options = [](bool args) {
    for (int i = 0; i < MAX_OPTION_ID; i++) {
      if (is_valid_option(i)) {
        if ((options[i].has_arg == required_argument) == args) {
          printf("--%s\n", options[i].name.c_str());
          if (options[i].short_name) {
            printf("-%c\n", options[i].short_name);
          }
        }
      }
    }
    exit(56);
  };

  while (true) {
    if (need_rebuild) {
      rebuild_options();
    }
    int option_index = 0;
    int c = getopt_long(argc, argv, opt_string.c_str(), long_opts.data(), &option_index);
    if (c == -1) {
      break;
    }
    if (c == PRINT_OPT_ARGS) {
      print_options(true);
    }
    if (c == PRINT_OPT_NO_ARGS) {
      print_options(false);
    }
    if (c == '?') {
      usage_and_exit();
    }
    if (!is_valid_option(c)) {
      c = get_option_alias(c);
      assert(c && is_valid_option(c));
    }
    int res;
    if (options[c].parser == nullptr) {
      res = execute ? execute(c, options[c].name.c_str()) : -1;
    } else {
      res = options[c].parser(c, options[c].name.c_str());
    }
    if (res < 0) {
      kprintf("Failed to parse option %s\n", options[c].name.c_str());
      usage_and_exit();
    }
  }
  return 0;
}

void always_enable_option(const char *name, char *arg) {
  need_rebuild = true;
  int c = find_parse_option_by_name(name);
  assert(c);
  assert(options[c].parser);
  char *old_optarg = optarg;
  optarg = arg;
  options[c].help += " [enabled by default]";
  options[c].parser(c, options[c].name.c_str());
  optarg = old_optarg;
}
