// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/src-dir.h"

#include <sys/stat.h>

SrcDir::SrcDir(std::string full_dir_name)
    : full_dir_name(std::move(full_dir_name)) {
  kphp_assert(this->full_dir_name[this->full_dir_name.size() - 1] == '/');

  has_modulite_yaml = access(get_modulite_yaml_filename().c_str(), F_OK) == 0;
  // has_composer_json is filled outside, when scanning vendor/, see register_composer_json()
}

std::string SrcDir::get_modulite_yaml_filename() const {
  return full_dir_name + ".modulite.yaml";
}
