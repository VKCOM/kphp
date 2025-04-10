// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <string>

#include "compiler/data/data_ptr.h"
#include "compiler/debug.h"

class LibData {
  DEBUG_STRING_METHOD {
    return lib_name_;
  }

public:
  LibData(const std::string& lib_name, const std::string& lib_dir);
  LibData(const std::string& lib_require_name);

  void update_lib_main_file(const std::string& main_file_path, const std::string& unified_lib_dir);

  static std::string run_global_function_name(const std::string& lib_name);

  std::string run_global_function_name() const;
  std::string static_archive_path() const;
  std::string headers_dir() const;
  std::string functions_txt_file() const;
  std::string runtime_lib_sha256_file() const;
  const std::string& lib_dir() const;
  const std::string& lib_namespace() const {
    return lib_name_;
  }
  const std::string& unified_lib_dir() const {
    return unified_lib_dir_;
  }
  bool is_raw_php() const {
    return is_raw_php_;
  }

  static const char* headers_tmp_dir();
  static const char* functions_txt_tmp_file();

private:
  std::string lib_dir_;
  std::string unified_lib_dir_;
  const std::string lib_name_;
  bool is_raw_php_{false};
};
