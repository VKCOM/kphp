#pragma once

#include "common/wrappers/string_view.h"

#include "compiler/data/data_ptr.h"

class SrcFile {
public:
  int id;
  std::string prefix;
  std::string text, file_name, short_file_name;
  std::string unified_file_name;
  std::string unified_dir_name;
  bool loaded;
  bool is_required;

  std::string main_func_name;
  FunctionPtr main_function;
  LibPtr owner_lib;

  vector<vk::string_view> lines;

  string namespace_name;                // namespace_name нужно унести на уровень файла (не функции), но пока не вышло до конца
  map<string, string> namespace_uses;   // use ... в начале файла — это per-file, а не per-function
  string class_context;

  SrcFile();
  SrcFile(const string &file_name, const string &short_file_name, const string &class_context, LibPtr owner_lib_id);
  void add_prefix(const string &s);
  bool load();

  vk::string_view get_line(int id);
  string get_short_name();
};
