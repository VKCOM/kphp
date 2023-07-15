// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"
//#include "compiler/make/cross-compile.h"

class Objs2ObjTarget : public Target {
public:
  std::string get_cmd() final {
    std::stringstream ss;
    ss << settings->cxx.get() <<
       " " << settings->cxx_toolchain_option.get() <<
//           gcc -dumpmachine
//       " --target=aarch64-pc-linux" <<
//       " --sysroot=/data/arm_sys_root" <<
       " " << settings->incremental_linker_flags.get() <<
       " -o " << output() <<
       " " << dep_list();
//    add_a(ss, settings->target.get(), settings->sys_root.get());
//    if (!settings->target.get().empty() && !settings->sys_root.get().empty()) {
//      ss << " --target=" << settings->target.get() << " --sysroot=" << settings->sys_root.get();
//    }
    if (!settings->sys_root.get().empty()) {
      ss << " --sysroot=" << settings->sys_root.get();
    }
    return ss.str();
  }
};
