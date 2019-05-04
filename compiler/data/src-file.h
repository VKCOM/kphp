#pragma once

#include <map>

#include "common/wrappers/string_view.h"

#include "compiler/data/data_ptr.h"

class SrcFile {
public:
  int id;
  std::string text, file_name, short_file_name;
  std::string unified_file_name;
  std::string unified_dir_name;
  bool loaded;
  bool is_required;

  std::string main_func_name;
  FunctionPtr main_function;
  LibPtr owner_lib;

  std::vector<vk::string_view> lines;

  std::string namespace_name;                // namespace_name нужно унести на уровень файла (не функции), но пока не вышло до конца
  std::map<std::string, std::string> namespace_uses;   // use ... в начале файла — это per-file, а не per-function

  SrcFile();
  SrcFile(const std::string &file_name, const std::string &short_file_name, LibPtr owner_lib_id);
  bool load();

  vk::string_view get_line(int id);
  std::string get_short_name();

  bool is_builtin() const;
};
