// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/stage.h"

#include "common/termformat/termformat.h"
#include "common/wrappers/pathname.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"
#include "compiler/threading/tls.h"
#include "compiler/utils/string-utils.h"

int stage::warnings_count;

const char *get_assert_level_desc(AssertLevelT assert_level) {
  switch (assert_level) {
    case WRN_ASSERT_LEVEL:
      return "Warning";
    case CE_ASSERT_LEVEL:
      return "Compilation error";
    case NOTICE_ASSERT_LEVEL:
      return "Notice";
    case FATAL_ASSERT_LEVEL:
      return "Fatal error";
    default:
      assert (0);
  }
}

volatile int ce_locker;

namespace {
FILE *warning_file{nullptr};
}

void stage::set_warning_file(FILE *file) noexcept {
  warning_file = file;
}

void on_compilation_error(const char *description __attribute__((unused)), const char *file_name, int line_number,
                          const char *full_description, AssertLevelT assert_level) {

  AutoLocker<volatile int *> locker(&ce_locker);
  FILE *file = stdout;
  if (assert_level == WRN_ASSERT_LEVEL && warning_file) {
    file = warning_file;
  }
  std::string error_title = get_assert_level_desc(assert_level);
  fmt_fprintf(file, "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n{} at stage: {}, gen by {}:{}\n",
              stage::should_be_colored(file) ? TermStringFormat::paint_red(error_title) : error_title, stage::get_name(), kbasename(file_name), line_number);
  stage::print_current_location_on_error(file);
  std::string correct_description = stage::should_be_colored(file) ? full_description : TermStringFormat::remove_special_symbols(full_description);
  fmt_fprintf(file, "\n{}\n\n", correct_description);
  if (assert_level == FATAL_ASSERT_LEVEL) {
    fmt_fprintf(file, "Compilation failed.\n"
                      "It is probably happened due to incorrect or unsupported PHP input.\n"
                      "But it is still bug in compiler.\n");
#ifdef __arm64__
    __builtin_debugtrap();  // for easier debugging kphp_assert / kphp_fail
#endif
    abort();
  }
  if (assert_level == CE_ASSERT_LEVEL) {
    stage::error();
  }
  if (assert_level == CE_ASSERT_LEVEL || assert_level == WRN_ASSERT_LEVEL) {
    stage::warnings_count++;
  }
  fflush(file);
}


void Location::set_file(SrcFilePtr new_file) {
  file = new_file;
  function = FunctionPtr();
  line = 0;
}

void Location::set_function(FunctionPtr new_function) {
  if (new_function) {
    file = new_function->file_id;
  }
  function = new_function;
  line = 0;
}

void Location::set_line(int new_line) {
  line = new_line;
}

Location::Location(const SrcFilePtr &file, const FunctionPtr &function, int line) :
  file(file),
  function(function),
  line(line) {}

// return a location in the format: "{file}:{line}  in {function}"
std::string Location::as_human_readable() const {
  std::string out;

  out += file ? file->relative_file_name : "unknown file";
  out += ":";
  out += std::to_string(line);

  // if it's a method of an PSR-4 class /path/to/A.php, output only A::methodName, not fully-qualified path\to\A::methodName
  // if we are inside a lambda, print out the outermost named function
  const auto *f_outer = function ? function->get_this_or_topmost_if_lambda() : nullptr;
  if (f_outer && f_outer->type == FunctionData::func_local) {
    std::string function_name = f_outer->as_human_readable();
    std::string psr4_file_name = replace_characters(function_name.substr(0, function_name.find(':')), '\\', '/') + ".php";
    if (file && file->relative_file_name == psr4_file_name) {
      function_name = function_name.substr(function_name.rfind('\\', psr4_file_name.size()) + 1);
    }
    out += function->is_lambda() ? "  in a lambda inside " + function_name : "  in " + function_name;
  } else if (f_outer && f_outer->type == FunctionData::func_main) {
    out += function->is_lambda() ? "  in a lambda in global scope" : "  in global scope";
  }
  return out;
}

const std::string &Location::calculate_batch_path_for_constant() const {
  if (!file) {
    return G->get_main_file()->relative_file_name;
  }
  if (file->relative_dir_name.empty()) {
    return file->relative_file_name;
  }
  return file->relative_dir_name;
}

bool operator<(const Location &lhs, const Location &rhs) {
  if (lhs.file && rhs.file) {
    if (const int cmp = lhs.file->file_name.compare(rhs.file->file_name)) {
      return cmp < 0;
    }
  } else if (lhs.file || rhs.file) {
    return static_cast<bool>(rhs.file);
  }
  if (lhs.line != rhs.line) {
    return lhs.line < rhs.line;
  }
  if (lhs.function && rhs.function) {
    return lhs.function->name < rhs.function->name;
  }
  return static_cast<bool>(rhs.function);
}

namespace stage {
static TLS<StageInfo> stage_info;
} // namespace stage

void stage::print_current_location_on_error(FILE *f) {
  bool use_colors = should_be_colored(f);
  Location location = get_location();

  // fix that sometimes (in strange cases) 'line' of location is not set, whereas current function exists
  // then let's point to a function declaration in error message
  if (location.line == 0 && location.function) {
    location.line = location.function->root->location.line;
  }

  // {file}:{line} in {function}
  fmt_fprintf(f, "  {}\n", location.as_human_readable());

  // line contents
  if (location.line > 0) {
    std::string comment = static_cast<std::string>(vk::trim(location.file->get_line(location.line)));
    comment = vk::replace_all(comment, "\n", "\\n");
    fmt_fprintf(f, "    {}\n", use_colors ? TermStringFormat::paint(comment, TermStringFormat::yellow) : comment);
  }
}

stage::StageInfo *stage::get_stage_info_ptr() {
  return &*stage_info;
}

void stage::set_name(std::string &&name) {
  get_stage_info_ptr()->name = std::move(name);
  get_stage_info_ptr()->cnt_errors = 0;
}

void stage::error() {
  get_stage_info_ptr()->global_error_flag = true;
  get_stage_info_ptr()->cnt_errors++;
}

bool stage::has_error() {
  return get_stage_info_ptr()->cnt_errors > 0;
}

bool stage::has_global_error() {
  for (int i = 0; i < stage_info.size(); i++) {
    if (stage_info.get(i).global_error_flag) {
      return true;
    }
  }
  return false;
}

void stage::die_if_global_errors() {
  if (stage::has_global_error()) {
    fmt_print("Compilation terminated due to errors\n");
    exit(1);
  }
}

const std::string &stage::get_name() {
  return get_stage_info_ptr()->name;
}

Location *stage::get_location_ptr() {
  return &get_stage_info_ptr()->location;
}

const Location &stage::get_location() {
  return *get_location_ptr();
}

void stage::set_location(const Location &new_location) {
  if (!new_location.get_file()) {
    return;
  }
  *get_location_ptr() = new_location;
}

void stage::set_file(SrcFilePtr file) {
  get_location_ptr()->set_file(file);
}

void stage::set_function(FunctionPtr function) {
  get_location_ptr()->set_function(function);
}

void stage::set_line(int line) {
  get_location_ptr()->set_line(line);
}

SrcFilePtr stage::get_file() {
  return get_location().get_file();
}

FunctionPtr stage::get_function() {
  return get_location().get_function();
}

int stage::get_line() {
  return get_location().get_line();
}

const std::string &stage::get_function_name() {
  static std::string no_function = "unknown";
  FunctionPtr function = get_function();
  if (!function) {
    return no_function;
  }
  return function->name;
}

bool stage::should_be_colored(FILE *f)  {
  if (!G) return TermStringFormat::is_terminal(f);
  switch (G->settings().get_color_settings()) {
    case CompilerSettings::colored:
      return true;
    case CompilerSettings::not_colored:
      return false;
    case CompilerSettings::auto_colored:
    default:
      return TermStringFormat::is_terminal(f);
  }
}
