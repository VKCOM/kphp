#pragma once

class Objs2BinTarget : public KphpTarget {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->get_cxx() << " -o " << target() << " -Wl,--whole-archive " << dep_list()
       << " -Wl,--no-whole-archive " << env->get_ld_flags();
    return ss.str();
  }
};
