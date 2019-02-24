#pragma once

#include "compiler/make/target.h"

class Cpp2ObjTarget : public KphpTarget {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->get_cxx() <<
       " -c -o " << target() <<
       " " << dep_list() <<
       " " << env->get_cxx_flags();

    if (get_file()->compile_with_debug_info_flag) {
      ss << " " << env->get_debug_level();
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
