// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/stage.h"

namespace tinf {

class RestrictionBase {
public:
  virtual ~RestrictionBase() = default;
  virtual const char *get_description() = 0;


  RestrictionBase() :
    location(stage::get_location()) {}

  virtual bool check_broken_restriction();

  static bool is_broken_restriction_an_error() { return true; }

protected:
  Location location;

  virtual bool check_broken_restriction_impl() = 0;
};

} // namespace tinf
