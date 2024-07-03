// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/make/target.h"

class RuntimeSrc2ObjTarget : public Target {
  std::string options;
public:
  RuntimeSrc2ObjTarget(std::string options)
    : options(std::move(options)) {}
  std::string get_cmd() final {
    std::stringstream ss;
    const auto cpp_list = dep_list();
    ss << settings->cxx.get() <<
      " -c -o " << target();
    ss << " " << dep_list() << " " << options;

    return ss.str();
  }
};