// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>

#include "common/wrappers/string_view.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class SrcFile {
public:
  int id{0};
  std::string text, file_name, short_file_name;
  std::string unified_file_name;
  std::string unified_dir_name;
  bool loaded{false};
  bool is_required{false};

  std::string main_func_name;
  FunctionPtr main_function;
  LibPtr owner_lib;

  std::vector<vk::string_view> lines;

  // TODO: namespace_name should probably be moved to a file level (as opposed to a function level)
  std::string namespace_name;
  // use ... in the beginning of the file — is a per-file construction, not a per-function one
  std::map<vk::string_view, vk::string_view> namespace_uses;

  SrcFile() = default;
  SrcFile(std::string file_name, std::string short_file_name, LibPtr owner_lib_id) :
    file_name(std::move(file_name)),
    short_file_name(std::move(short_file_name)),
    owner_lib(owner_lib_id) {
  }

  bool load();

  vk::string_view get_line(int id);
  std::string get_short_name();

  bool is_builtin() const;
  std::string get_main_func_run_var_name() const;
  VertexPtr get_main_func_run_var() const;
};
