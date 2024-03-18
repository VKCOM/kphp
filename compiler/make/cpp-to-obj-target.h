// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "common/algorithms/contains.h"

#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"

class Cpp2ObjTarget : public Target {
public:
  std::string get_cmd() final {
    std::stringstream ss;
    const auto cpp_list = dep_list();
    ss << settings->cxx.get() << " -c -o " << target() << " " << cpp_list;
    const auto &cxx_flags = get_file()->compile_with_debug_info_flag ? settings->cxx_flags_with_debug : settings->cxx_flags_default;
    // make #include "runtime-headers.h" capture generated pch file
    // it's done via -iquote to a folder inside /tmp/kphp_gch where runtime-headers.h with pch file are placed
    if (!settings->no_pch.get()) {
      ss << " -iquote " << cxx_flags.pch_dir.get();
      if (vk::contains(settings->cxx.get(), "clang")) {
        ss << " -include " << cxx_flags.pch_dir.get() << settings->runtime_headers.get();
      }
    }
    ss << " " << cxx_flags.flags.get();

    return ss.str();
  }

  void compute_priority() final {
    priority = 0;
    for (auto *dep : deps) {
      if (File *dep_file = dep->get_file()) {
        priority += dep_file->file_size;
      }
    }
  }
};
