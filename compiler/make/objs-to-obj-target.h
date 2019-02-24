#pragma once

class Objs2ObjTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->ld <<
       " -r" <<
       " -o" << target() <<
       " " << dep_list();
    return ss.str();
  }
};
