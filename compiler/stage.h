// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <unistd.h>

#include "common/exit-codes.h"

#include "compiler/data/data_ptr.h"
#include "compiler/kphp_assert.h"
#include "compiler/location.h"

inline void on_compilation_error(const char *description, const char *file_name, int line_number, const std::string &full_description, AssertLevelT assert_level) {
  on_compilation_error(description, file_name, line_number, full_description.c_str(), assert_level);
}

namespace stage {

void set_warning_file(FILE *file) noexcept;

struct StageInfo {
  std::string name;
  Location location;
  bool global_error_flag{false};
  uint32_t cnt_errors{0};
  ExitCode exit_code = ExitCode::KPHP_TO_CPP_STAGE;
};

StageInfo *get_stage_info_ptr();

void error();
bool has_error();
bool has_global_error();
void die_if_global_errors();

Location *get_location_ptr();
const Location &get_location();
void set_location(const Location &new_location);

void print_current_location_on_error(FILE *f);

void set_name(std::string &&name);
const std::string &get_name();

void set_exit_code(ExitCode code);
ExitCode get_exit_code();

void set_file(SrcFilePtr file);
void set_function(FunctionPtr function);
void set_line(int line);
SrcFilePtr get_file();
FunctionPtr get_function();
int get_line();

const std::string &get_function_name();
bool should_be_colored(FILE *f);

extern int warnings_count;
} // namespace stage


