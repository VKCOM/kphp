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
    ss << env->cxx <<
       " -c -o " << target() <<
       " " << cpp_type << cpp_list <<
       " " << env->cxx_flags;

    if (get_file()->compile_with_debug_info_flag) {
      ss << " " << env->debug_level;
    }

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
