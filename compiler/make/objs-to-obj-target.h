// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"

class Objs2ObjTarget : public Target {
bool force_obj;

public:
  explicit Objs2ObjTarget(bool force_obj_) : force_obj(force_obj_) {}
  
  std::string get_cmd() final {
    std::stringstream ss;
    ss << settings->cxx.get() <<
       " " << settings->cxx_toolchain_option.get() <<
       " " << 
      //  settings->incremental_linker_flags.get() <<
      (force_obj ? "-r -nostdlib" : settings->incremental_linker_flags.get())
       << 
       " -o " << target() <<
       " " << dep_list();
    return ss.str();
  }
};
