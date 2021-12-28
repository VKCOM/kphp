// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "common/algorithms/contains.h"

#include "compiler/make/target.h"

class Cpp2ObjTarget : public Target {
public:
  explicit Cpp2ObjTarget(bool make_pch = false) noexcept :
    make_pch_(make_pch) {
  }

  string get_cmd() final {
    std::stringstream cmd;
    const auto cpp_list = dep_list();
    const char *cpp_type = make_pch_ ? "-x c++-header " : "";
    cmd << settings->cxx.get() << " -c -o " << target() << " " << cpp_type << cpp_list;

    if (!make_pch_ && !settings->no_pch.get()) {
      cmd << " -iquote " << settings->pch_dir.get();
      if (vk::contains(settings->cxx.get(), "clang")) {
        cmd << " -include " << settings->pch_dir.get() << settings->runtime_headers.get();
      }
    }

    const auto &cxx_flags = get_file()->compile_with_debug_info_flag ? settings->cxx_flags_with_debug : settings->cxx_flags_default;
    cmd << " " << cxx_flags.flags.get();

    return cmd.str();
  }

  void compute_priority() final {
    priority = 0;
    for (auto *dep : deps) {
      if (File *dep_file = dep->get_file()) {
        priority += dep_file->file_size;
      }
    }
  }

private:
  bool make_pch_{false};
};
