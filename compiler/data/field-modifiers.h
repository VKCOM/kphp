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
  explicit FieldModifiers(AccessModifiers access_modifiers)
      : access_modifier_(access_modifiers) {}

  FieldModifiers& set_public() {
    kphp_error(!is_private() && !is_protected(), "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::public_;
    return *this;
  }

  FieldModifiers& set_private() {
    kphp_error(!is_public() && !is_protected(), "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::private_;
    return *this;
  }

  FieldModifiers& set_protected() {
    kphp_error(!is_private() && !is_public(), "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::protected_;
    return *this;
  }

  bool is_public() const {
    return access_modifier_ == AccessModifiers::public_;
  }
  bool is_private() const {
    return access_modifier_ == AccessModifiers::private_;
  }
  bool is_protected() const {
    return access_modifier_ == AccessModifiers::protected_;
  }

  std::string to_string() const {
    switch (access_modifier_) {
    case AccessModifiers::public_:
      return "public";
    case AccessModifiers::private_:
      return "private";
    case AccessModifiers::protected_:
      return "protected";
    default:
      return "";
    }
  }

  friend bool operator==(const FieldModifiers& lhs, const FieldModifiers& rhs) {
    return lhs.access_modifier_ == rhs.access_modifier_;
  }

private:
  AccessModifiers access_modifier_{AccessModifiers ::not_modifier_};
};
