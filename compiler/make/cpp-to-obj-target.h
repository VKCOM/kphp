#pragma once

#include <sstream>

#include "compiler/make/target.h"

class Cpp2ObjTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->cxx <<
       " -c -o " << target() <<
       " " << dep_list() <<
       " " << env->cxx_flags;

    if (get_file()->compile_with_debug_info_flag) {
      ss << " " << env->debug_level;
    }

    return ss.str();
  }
  void compute_priority() {
    struct stat st;
    priority = 0;
    if (stat(get_name().c_str(), &st) == 0) {
      priority = st.st_size;
    }
  }
};
