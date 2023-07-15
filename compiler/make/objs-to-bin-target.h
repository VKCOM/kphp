// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"
//#include "cross-compile.h"

class Objs2BinTarget : public Target {
public:
  explicit Objs2BinTarget(bool need_libdl = false) noexcept :
    need_libdl_{need_libdl} {
  }

  std::string get_cmd() final {
#if defined(__APPLE__)
    std::string_view open_dep{" -Wl,-all_load "};
    std::string_view close_dep;
#else
    std::string_view open_dep{" -Wl,--whole-archive -Wl,--start-group "};
    std::string_view close_dep{" -Wl,--end-group -Wl,--no-whole-archive "};
#endif
    std::stringstream ss;
    ss << settings->cxx.get() << " " << settings->cxx_toolchain_option.get()
       << " -o " << output() << open_dep << dep_list() << close_dep << settings->ld_flags.get();
//    add_a(ss, settings->target.get(), settings->sys_root.get());
//    if (!settings->target.get().empty() && !settings->sys_root.get().empty()) {
//      ss << " --target=" << settings->target.get() << " --sysroot=" << settings->sys_root.get();
//    }
    if (!settings->sys_root.get().empty()) {
      ss << " --sysroot=" << settings->sys_root.get();
    }
    if (need_libdl_) {
      ss << " -ldl";
    }
    return ss.str();
  }

private:
  bool need_libdl_{false};
};
