#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/location.h"

#define compiler_assert(x, y, level)  ({\
  int kphp_error_res__ = 0;\
  if (!(x)) {\
    kphp_error_res__ = 1;\
    on_compilation_error (#x, __FILE__, __LINE__, y, level);\
  }\
  kphp_error_res__;\
})

#define kphp_warning(y)  compiler_assert (0, y, WRN_ASSERT_LEVEL)
#define kphp_typed_warning(x, y) do {                                      \
  FunctionPtr kphp_warning_fun__ = stage::get_function();                  \
  if (kphp_warning_fun__) {                                     \
    const set<string> &disabled__ = kphp_warning_fun__->disabled_warnings; \
    string s = x;                                                          \
    if (disabled__.find(s) == disabled__.end()) {                          \
      string message = y;                                                  \
      message += "\n   (Can be disabled by adding '" + s +                 \
                 "' to @kphp-disable-warnings attribute of function)";     \
      compiler_assert (0, message.c_str(), WRN_ASSERT_LEVEL);              \
    }                                                                      \
  }                                                                        \
} while (0)
#define kphp_error(x, y) compiler_assert (x, y, CE_ASSERT_LEVEL)
#define kphp_error_act(x, y, act) if (kphp_error (x, y)) act;
#define kphp_error_return(x, y) kphp_error_act (x, y, return)
#define kphp_assert(x) compiler_assert (x, "", FATAL_ASSERT_LEVEL)
#define kphp_assert_msg(x, y) compiler_assert (x, y, FATAL_ASSERT_LEVEL)
#define kphp_fail() kphp_assert (0); _exit(1);

enum AssertLevelT {
  WRN_ASSERT_LEVEL,
  CE_ASSERT_LEVEL,
  FATAL_ASSERT_LEVEL
};

void on_compilation_error(const char *description, const char *file_name, int line_number,
                          const char *full_description, AssertLevelT assert_level);

namespace stage {
struct StageInfo {
  string name;
  Location location;
  bool global_error_flag;
  bool error_flag;

  StageInfo() :
    name(),
    location(),
    global_error_flag(false),
    error_flag(false) {
  }
};

StageInfo *get_stage_info_ptr();

void error();
bool has_error();
bool has_global_error();
void die_if_global_errors();

Location *get_location_ptr();
const Location &get_location();
void set_location(const Location &new_location);

void print(FILE *f);
void print_file(FILE *f);
void print_function(FILE *f);
void print_line(FILE *f);
void print_comment(FILE *f);

void set_name(const string &name);
const string &get_name();

void set_file(SrcFilePtr file);
void set_function(FunctionPtr file);
void set_line(int line);
SrcFilePtr get_file();
FunctionPtr get_function();
int get_line();

const string &get_file_name();
const string &get_function_name();
string to_str(const Location &new_location);
bool should_be_colored(FILE *f);

extern int warnings_count;
} // namespace stage


