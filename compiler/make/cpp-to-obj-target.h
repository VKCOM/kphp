// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "common/algorithms/contains.h"

#include "compiler/make/target.h"

class Cpp2ObjTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    const auto cpp_list = dep_list();
    const char *cpp_type = vk::contains(cpp_list, ".h") ? "-x c++-header " : "";
    ss << settings->cxx.get() <<
       " -c -o " << target() <<
       " " << cpp_type << cpp_list;
    const auto &cxx_flags = get_file()->compile_with_debug_info_flag ? settings->cxx_flags_with_debug : settings->cxx_flags_default;
    if (!settings->no_pch.get()) {
      ss << " -iquote" << cxx_flags.pch_dir.get();
    }
    ss << " " << cxx_flags.flags.get();

    return ss.str();
  }

  void compute_priority() final {
    priority = 0;
    for (auto dep : deps) {
      if (File *dep_file = dep->get_file()) {
        priority += dep_file->file_size;
      }
    }
  }
};
