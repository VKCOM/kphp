// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

#include "compiler/data/class-member-modifiers.h"
#include "compiler/stage.h"

class ClassModifiers {
public:
  void set_final() {
    kphp_error(!is_abstract(), "Can not use final and abstract at the same time");
    abstract_modifier_ = AbstractModifiers::final_;
  }

  void set_abstract() {
    kphp_error(!is_final(), "Can not use final and abstract at the same time");
    abstract_modifier_ = AbstractModifiers::abstract_;
  }

  bool is_final() const {
    return abstract_modifier_ == AbstractModifiers::final_;
  }
  bool is_abstract() const {
    return abstract_modifier_ == AbstractModifiers::abstract_;
  }

private:
  AbstractModifiers abstract_modifier_{AbstractModifiers::not_modifier_};
};
