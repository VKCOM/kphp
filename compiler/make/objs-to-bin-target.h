#pragma once

class Objs2BinTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->cxx << " -o " << target() << " -Wl,--whole-archive " << dep_list()
       << " -Wl,--no-whole-archive " << env->ld_flags;
    return ss.str();
  }
};
