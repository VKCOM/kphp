#pragma once

class Objs2ObjTarget : public KphpTarget {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->get_ld() <<
       " -r" <<
       " -o" << target() <<
       " " << dep_list();
    return ss.str();
  }
};
