// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>

#include "common/wrappers/string_view.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"

class SrcFile {
  DEBUG_STRING_METHOD {
    return relative_file_name;
  }

public:
  int id{0};
  std::string text, file_name, short_file_name;
  std::string relative_file_name;
  std::string relative_dir_name;
  bool loaded{false};
  bool is_required{false};
  bool is_strict_types{false};        // whether declare(strict_types=1) is set
  bool is_from_functions_file{false}; // functions.txt + all files required from it
  bool is_loaded_by_psr0{false};

  std::string main_func_name;
  FunctionPtr main_function;
  SrcDirPtr dir;
  LibPtr owner_lib;

  std::vector<vk::string_view> lines;

  std::string namespace_name;
  // 'use' mappings from the beginning of the file
  std::map<vk::string_view, vk::string_view> namespace_uses;

  SrcFile() = default;
  SrcFile(std::string file_name, std::string short_file_name, LibPtr owner_lib_id)
    : file_name(std::move(file_name))
    , short_file_name(std::move(short_file_name))
    , owner_lib(owner_lib_id) {}

  bool load();

  vk::string_view get_line(int id);

  bool is_builtin() const {
    return is_from_functions_file || is_from_owner_lib();
  }
  bool is_from_owner_lib() const;

  std::string get_main_func_run_var_name() const;
  VertexPtr get_main_func_run_var() const;
};
