#pragma once

class Objs2StaticLibTarget : public KphpTarget {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->get_ar() << " rcs " << target() << " " << dep_list();
    return ss.str();
  }
};
