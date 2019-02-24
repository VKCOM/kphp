#pragma once

class Objs2StaticLibTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->ar << " rcs " << target() << " " << dep_list();
    return ss.str();
  }
};
