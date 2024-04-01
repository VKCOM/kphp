// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/make/target.h"

class FileTarget : public Target {
public:
  std::string get_cmd() final {
    assert(0);
    return "";
  }
};
