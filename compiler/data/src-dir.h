// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <string>

#include "common/wrappers/string_view.h"

#include "compiler/data/data_ptr.h"
#include "compiler/debug.h"

class SrcDir {
  DEBUG_STRING_METHOD {
    return full_dir_name;
  }

public:
  explicit SrcDir(std::string full_dir_name);

  std::string full_dir_name;
  SrcDirPtr parent_dir;
  bool has_modulite_yaml{false};
  bool has_composer_json{false};
  ModulitePtr nested_files_modulite;

  enum class PassStatus {
    uninitialized,
    processing,
    done,
  };
  std::atomic<PassStatus> state_collect_required{PassStatus::uninitialized}; // for "collect required" pass

  std::string get_modulite_yaml_filename() const;
};
