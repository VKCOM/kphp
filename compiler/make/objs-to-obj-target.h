#pragma once

class Objs2ObjTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->incremental_linker <<
       " " << env->incremental_linker_flags <<
       " -o " << target() <<
       " " << dep_list();
    return ss.str();
  }
};
