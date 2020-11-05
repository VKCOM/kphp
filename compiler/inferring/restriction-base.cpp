// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/restriction-base.h"

bool tinf::RestrictionBase::check_broken_restriction() {
  bool err = check_broken_restriction_impl();
  if (err) {
    stage::set_location(location);
    if (is_broken_restriction_an_error()) {
      kphp_error (0, get_description());
    } else {
      kphp_warning (get_description());
    }
  }

  return err;
}
