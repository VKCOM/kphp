// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sstream>

#include "compiler/compiler-settings.h"
#include "compiler/make/target.h"

class Objs2StaticLibTarget : public Target {
public:
  std::string get_cmd() final {
    std::stringstream ss;
    ss << settings->archive_creator.get() << " rcs " << output() << " " << dep_list();
    return ss.str();
  }
};
