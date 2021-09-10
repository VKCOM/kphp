// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

#include "compiler/data/class-member-modifiers.h"
#include "compiler/stage.h"

class FieldModifiers {
public:
  FieldModifiers() = default;

  explicit FieldModifiers(AccessModifiers access_modifiers) :
    access_modifier_(access_modifiers) {
  }

  FieldModifiers(AccessModifiers access_modifiers, AbstractModifiers abstract_modifiers) :
    abstract_modifier_(abstract_modifiers), access_modifier_(access_modifiers) {
  }

  FieldModifiers &set_public() {
    kphp_error(!is_private() && !is_protected(), "Multiple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::public_;
    return *this;
  }

  FieldModifiers &set_private() {
    kphp_error(!is_public() && !is_protected(), "Multiple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::private_;
    return *this;
  }

  FieldModifiers &set_protected() {
    kphp_error(!is_private() && !is_public(), "Multiple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::protected_;
    return *this;
  }

  FieldModifiers &set_readonly() {
    abstract_modifier_ = AbstractModifiers::readonly_;
    return *this;
  }

  bool is_public()    const { return access_modifier_ == AccessModifiers::public_;       }
  bool is_private()   const { return access_modifier_ == AccessModifiers::private_;      }
  bool is_protected() const { return access_modifier_ == AccessModifiers::protected_;    }
  bool is_readonly()  const { return abstract_modifier_ == AbstractModifiers::readonly_; }

  std::string to_string() const {
    std::string res;
    switch (access_modifier_) {
      case AccessModifiers::public_:
        res = "public";
        break;
      case AccessModifiers::private_ :
        res = "private";
        break;
      case AccessModifiers::protected_:
        res = "protected";
        break;
      default:
        break;
    }

    if (is_readonly()) {
      if (res.empty()) {
        res += "readonly";
      } else {
        res += " readonly";
      }
    }

    return res;
  }

  friend bool operator==(const FieldModifiers &lhs, const FieldModifiers &rhs) {
    return lhs.access_modifier_ == rhs.access_modifier_;
  }

private:
  AbstractModifiers abstract_modifier_{AbstractModifiers ::readonly_};
  AccessModifiers access_modifier_{AccessModifiers ::not_modifier_};
};
