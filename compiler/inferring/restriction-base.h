// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/stage.h"

namespace tinf {

class RestrictionBase {
public:
  virtual ~RestrictionBase() = default;

  RestrictionBase()
    : location(stage::get_location()) {}

  const Location &get_location() const {
    return location;
  }

  virtual bool is_restriction_broken() = 0;
  virtual std::string get_description() = 0;

protected:
  Location location;
};

} // namespace tinf
