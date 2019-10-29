#include "compiler/stage.h"

#include <regex>
#include <sstream>

#include "common/termformat/termformat.h"

#include "compiler/common.h"
#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/threading/tls.h"
#include "compiler/utils/string-utils.h"

int stage::warnings_count;

const char *get_assert_level_desc(AssertLevelT assert_level) {
  switch (assert_level) {
    case WRN_ASSERT_LEVEL:
      return "Warning";
    case CE_ASSERT_LEVEL:
      return "Compilation error";
    case FATAL_ASSERT_LEVEL:
      return "Fatal error";
    default:
      assert (0);
  }
}

volatile int ce_locker;

void on_compilation_error(const char *description __attribute__((unused)), const char *file_name, int line_number,
                          const char *full_description, AssertLevelT assert_level) {

  AutoLocker<volatile int *> locker(&ce_locker);
  FILE *file = stdout;
  if (assert_level == WRN_ASSERT_LEVEL && G->env().get_warnings_file()) {
    file = G->env().get_warnings_file();
  }
  fmt_fprintf(file, "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n{} [gen by {} at {}]\n", get_assert_level_desc(assert_level), file_name, line_number);
  stage::print(file);
  string correct_description;
  if (!stage::should_be_colored(file)) {
    correct_description = TermStringFormat::remove_special_symbols(full_description);
  } else {
    correct_description = full_description;
  }
  fmt_fprintf(file, "{}\n\n", correct_description);
  if (assert_level == FATAL_ASSERT_LEVEL) {
    fmt_fprintf(file, "Compilation failed.\n"
                      "It is probably happened due to incorrect or unsupported PHP input.\n"
                      "But it is still bug in compiler.\n");
    abort();
  }
  if (assert_level == CE_ASSERT_LEVEL) {
    stage::error();
  }
  stage::warnings_count++;
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

SrcFilePtr Location::get_file() const {
  return file;
}

FunctionPtr Location::get_function() const {
  return function;
}

int Location::get_line() const {
  return line;
}
Location::Location(const SrcFilePtr &file, const FunctionPtr &function, int line) :
  file(file),
  function(function),
  line(line) {}

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

void stage::print(FILE *f) {
  fmt_fprintf(f, "In stage = [{}]:\n", get_name());
  fmt_fprintf(f, "  ");
  print_file(f);
  fmt_fprintf(f, "  ");
  print_function(f);
  fmt_fprintf(f, "  ");
  print_line(f);
  print_comment(f);
}

void stage::print_file(FILE *f) {
  fmt_fprintf(f, "[file = {}]\n", get_file_name());
}

void stage::print_function(FILE *f) {
  FunctionPtr function = get_function();
  string function_name_str = (function ? function->get_human_readable_name() : "unknown function");
  if (should_be_colored(f)) {
    function_name_str = TermStringFormat::add_text_attribute(function_name_str, TermStringFormat::bold);
  }
  fmt_fprintf(f, "[function = {}]\n", function_name_str);
}

void stage::print_line(FILE *f) {
  if (get_line() > 0) {
    fmt_fprintf(f, "[line = {}]\n", get_line());
  }
}

void stage::print_comment(FILE *f) {
  if (get_line() > 0) {
    vk::string_view comment = get_file()->get_line(get_line());
    fmt_fprintf(f, "//{:4}:", get_line());
    int last_printed = ':';
    for (int j = 0, nj = comment.size(); j < nj; j++) {
      int c = comment.begin()[j];
      if (c == '\n') {
        putc('\\', f);
        putc('n', f);
        last_printed = 'n';
      } else if (c > 13) {
        putc(c, f);
        if (c > 32) {
          last_printed = c;
        }
      }
    }
    if (last_printed == '\\') {
      putc(';', f);
    }
    putc('\n', f);
  }
}

stage::StageInfo *stage::get_stage_info_ptr() {
  return &*stage_info;
}

void stage::set_name(const string &name) {
  get_stage_info_ptr()->name = name;
  get_stage_info_ptr()->error_flag = false;
}

void stage::error() {
  get_stage_info_ptr()->global_error_flag = true;
  get_stage_info_ptr()->error_flag = true;
}

bool stage::has_error() {
  return get_stage_info_ptr()->error_flag;
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

const string &stage::get_name() {
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

void stage::set_function(FunctionPtr file) {
  get_location_ptr()->set_function(file);
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

const string &stage::get_file_name() {
  static string no_file = "unknown";
  SrcFilePtr file = get_file();
  if (!file) {
    return no_file;
  }
  return file->file_name;
}

const string &stage::get_function_name() {
  static string no_function = "unknown";
  FunctionPtr function = get_function();
  if (!function) {
    return no_function;
  }
  return function->name;
}

string stage::to_str(const Location &new_location) {
  set_location(new_location);
  FunctionPtr function = get_function();
  std::stringstream ss;

  // Модифицировать вывод осторожно! По некоторым символам используется поиск регекспами при выводе стектрейса
  ss << (get_file() ? get_file()->get_short_name() : "unknown file") << ": " << (function ? function->get_human_readable_name() : "unknown function") << ":" << get_line();
  std::string out = ss.str();

  // Убираем дублирование имени класса в пути до класса
  std::smatch matched;
  if (std::regex_match(out, matched, std::regex("(.+?): ((.*?)::.*)"))) {
    string class_name = replace_characters(matched[3].str(), '\\', '/');
    if (matched[1].str().find(class_name + ".php") == matched[1].str().length() - (class_name.length() + 4)) {
      return matched[2].str();
    }
  }

  return out;
}

bool stage::should_be_colored(FILE *f)  {
  switch (G->env().get_color_settings()) {
    case KphpEnviroment::colored:
      return true;
    case KphpEnviroment::not_colored:
      return false;
    case KphpEnviroment::auto_colored:
    default:
      return TermStringFormat::is_terminal(f);
  }
}
