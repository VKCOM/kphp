// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/lib-data.h"

#include "common/wrappers/string_view.h"
#include "compiler/data/src-file.h"
#include "compiler/stage.h"

namespace {
std::string get_lib_dir(const std::string &main_file_path) {
  const std::size_t last_slash = main_file_path.rfind('/');
  kphp_assert_msg(last_slash != std::string::npos, "bad lib main file path");
  return main_file_path.substr(0, last_slash + 1);
}

std::string get_lib_name(const std::string &lib_require_name) {
  // expected: .../?${name}
  const std::size_t start_pos = lib_require_name.rfind('/');
  std::string lib_name = start_pos == std::string::npos ? lib_require_name : lib_require_name.substr(start_pos + 1);
  kphp_assert_msg(!lib_name.empty(), "bad lib require name");
  return lib_name;
}

bool is_raw_php_dir(vk::string_view lib_dir) {
  return lib_dir.ends_with("/php/");
}
} // namespace

LibData::LibData(const std::string &lib_name, const std::string &lib_dir)
  : lib_dir_(lib_dir)
  , lib_name_(lib_name) {}

LibData::LibData(const std::string &lib_require_name)
  : lib_name_(get_lib_name(lib_require_name)) {}

void LibData::update_lib_main_file(const std::string &main_file_path, const std::string &unified_lib_dir) {
  kphp_assert_msg(lib_dir_.empty(), "lib directory is known");
  lib_dir_ = get_lib_dir(main_file_path);
  unified_lib_dir_ = unified_lib_dir;
  is_raw_php_ = is_raw_php_dir(lib_dir_);
}

std::string LibData::run_global_function_name(const std::string &lib_name) {
  return lib_name + "_run_global";
}

std::string LibData::static_archive_path() const {
  kphp_assert_msg(!is_raw_php(), "should not be called for raw php lib");
  std::string lib_path = lib_dir();
  lib_path.append("lib").append(lib_name_).append(".a");
  return lib_path;
}

std::string LibData::run_global_function_name() const {
  kphp_assert_msg(!is_raw_php(), "should not be called for raw php lib");
  return run_global_function_name(lib_name_);
}

std::string LibData::headers_dir() const {
  kphp_assert_msg(!is_raw_php(), "should not be called for raw php lib");
  return lib_dir() + "headers/";
}

std::string LibData::functions_txt_file() const {
  kphp_assert_msg(!is_raw_php(), "should not be called for raw php lib");
  return lib_dir() + "functions.txt";
}

std::string LibData::runtime_lib_sha256_file() const {
  kphp_assert_msg(!is_raw_php(), "should not be called for raw php lib");
  return lib_dir() + "php_lib_version.sha256";
}

const std::string &LibData::lib_dir() const {
  kphp_assert_msg(!lib_dir_.empty(), "lib directory is unknown");
  return lib_dir_;
}

const char *LibData::headers_tmp_dir() {
  return "lib_headers/";
}

const char *LibData::functions_txt_tmp_file() {
  return "lib_functions.txt";
}
