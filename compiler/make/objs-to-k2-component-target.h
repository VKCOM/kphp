// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "auto/compiler/runtime_link_libs.h"
#include "compiler/compiler-core.h"
#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"
#include "compiler/utils/string-utils.h"

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

  static std::string additional_flags() {
#if defined(__APPLE__)
    return " -Wl,-undefined,dynamic_lookup ";
#else
    return " -Wl,--wrap,malloc -Wl,--wrap,free, -Wl,--wrap,calloc -Wl,--wrap,realloc -static-libgcc";
#endif
  }

public:
  std::string get_cmd() final {
    std::stringstream ss;
    ss << settings->cxx.get() << additional_flags() << " -stdlib=libc++ -static-libstdc++ -shared -o " << target() << " ";

    for (size_t i = 0; i + 1 < deps.size(); ++i) {
      ss << deps[i]->get_name() << " ";
    }

    // the last dep is runtime lib
    // todo:k2 think about kphp-libraries
    assert(deps.size() >= 1 && "There are should be at least one dependency. It's the runtime lib");
    if (G->settings().force_link_runtime.get()) {
      ss << load_all_symbols_pre() << deps.back()->get_name() << load_all_symbols_post();
    } else {
      ss << deps.back()->get_name() << " ";
    }
    // add vendored statically linking libs
    std::vector<std::string> libs = split(RUNTIME_LINK_LIBS);
    std::for_each(libs.cbegin(), libs.cend(), [&ss](const auto &lib) noexcept { ss << lib << " "; });
    return ss.str();
  }
};
