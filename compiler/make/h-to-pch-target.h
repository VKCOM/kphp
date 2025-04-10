// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "common/algorithms/contains.h"

#include "compiler/compiler-settings.h"
#include "compiler/kphp_assert.h"
#include "compiler/make/target.h"

class H2PchTarget : public Target {
public:
  std::string get_cmd() final {
    kphp_assert(deps.size() == 1);

    std::stringstream ss;
    const auto& cxx_flags = get_file()->compile_with_debug_info_flag ? settings->cxx_flags_with_debug : settings->cxx_flags_default;
    ss << settings->cxx.get() << " -c -o " << target() << " -x c++-header " << deps[0]->get_name() << " " << cxx_flags.flags.get();

    // printf("compile pch cmd line %s\n", ss.str().c_str());
    return ss.str();
  }

  bool after_run_success() override {
    long long res = get_file()->read_stat();
    if (res < 0) {
      return false;
    } else if (res > 0) {
      upd_mtime(get_file()->mtime);
    }
    return true;
  }
};
