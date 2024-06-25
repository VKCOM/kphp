// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"

class Objs2K2ComponentTarget : public Target {
  static std::string load_all_symbols_pre() {
#if defined(__APPLE__)
    return "-Wl,-force_load ";
#else
    return "-Wl,--whole-archive ";
#endif
  }

  static std::string load_all_symbols_post() {
#if defined(__APPLE__)
    return " ";
#else
    return " -Wl,--no-whole-archive ";
#endif
  }

public:
  std::string get_cmd() final {
    std::stringstream ss;
    ss << settings->cxx.get() << " -static-libgcc -stdlib=libc++ -static-libstdc++ -shared -o " << target() << " ";

    for (size_t i = 0; i + 1 < deps.size(); ++i) {
      ss << deps[i]->get_name() << " ";
    }

    // the last dep is runtime lib
    // todo:k2 think about kphp-libraries
    assert(deps.size() >= 1 && "There are should be at least one dependency. It's the runtime lib");
    if (!G->settings().rt_from_sources.get()) {
      ss << load_all_symbols_pre() << deps.back()->get_name() << load_all_symbols_post();
    } else {
      ss << deps.back()->get_name() << " ";
    }
    return ss.str();
  }
};
