// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/data/class-member-modifiers.h"
#include "compiler/kphp_assert.h"

class FunctionModifiers {
private:
  FunctionModifiers() = default;

public:
  FunctionModifiers(const FunctionModifiers &) = default;
  FunctionModifiers& operator=(const FunctionModifiers &) = default;
  ~FunctionModifiers() = default;

  void set_static() {
    kphp_assert(!is_instance());
    scope_modifier_ = ScopeModifiers::static_;
  }

  void set_instance() {
    kphp_assert(!is_static());
    scope_modifier_ = ScopeModifiers::instance_;
  }

  void set_public() {
    kphp_error(!is_private() && !is_protected(), "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::public_;
  }

  void set_private() {
    kphp_error(!is_public() && !is_protected(), "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::private_;
  }

  void set_protected() {
    kphp_error(!is_private() && !is_public(), "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
    access_modifier_ = AccessModifiers::protected_;
  }

  void set_final() {
    kphp_error(!is_abstract(), "Can not use final and abstract at the same time");
    abstract_modifier_ = AbstractModifiers::final_;
  }

  void set_abstract() {
    kphp_error(!is_final(), "Can not use final and abstract at the same time");
    abstract_modifier_ = AbstractModifiers::abstract_;
  }

  bool is_static()    const { return scope_modifier_ == ScopeModifiers::static_;   }
  bool is_instance()  const { return scope_modifier_ == ScopeModifiers::instance_; }
  bool is_nonmember() const { return !is_static() && !is_instance();   }
  
  bool is_static_lambda() const { return scope_modifier_ == ScopeModifiers::static_lambda_; }

  bool is_public()    const { return access_modifier_ == AccessModifiers::public_;    }
  bool is_private()   const { return access_modifier_ == AccessModifiers::private_;   }
  bool is_protected() const { return access_modifier_ == AccessModifiers::protected_; }

  bool is_final()    const { return abstract_modifier_ == AbstractModifiers::final_; }
  bool is_abstract() const { return abstract_modifier_ == AbstractModifiers::abstract_;    }

  std::string to_string() const {
    std::string res;
    if (is_final())    res += "final";
    if (is_abstract()) res += "abstract";

    if (is_public())    res += " public";
    if (is_private())   res += " private";
    if (is_protected()) res += " protected";

    if (is_static())    res += " static";
    if (is_instance())  res += " instance";
    if (is_nonmember()) res += " nonmember";

    return res;
  }

  AccessModifiers access_modifier() const {
    return access_modifier_;
  }

  static FunctionModifiers instance_public() {
    FunctionModifiers res;
    res.set_instance();
    res.set_public();

    return res;
  }

  static FunctionModifiers instance_private() {
    FunctionModifiers res;
    res.set_instance();
    res.set_private();

    return res;
  }

  static FunctionModifiers static_lambda() {
    FunctionModifiers res;
    res.scope_modifier_ = ScopeModifiers::static_lambda_;

    return res;
  }

  static FunctionModifiers nonmember() {
    return FunctionModifiers{};
  }

  friend bool operator==(const FunctionModifiers &lhs, const FunctionModifiers &rhs) {
    return lhs.abstract_modifier_ == rhs.abstract_modifier_ &&
           lhs.scope_modifier_    == rhs.scope_modifier_    &&
           lhs.access_modifier_   == rhs.access_modifier_;
  }

  friend bool operator!=(const FunctionModifiers &lhs, const FunctionModifiers &rhs) {
    return !(lhs == rhs);
  }

private:
  AbstractModifiers abstract_modifier_{AbstractModifiers::not_modifier_};
  ScopeModifiers scope_modifier_{ScopeModifiers::not_member_};
  AccessModifiers access_modifier_{AccessModifiers ::not_modifier_};
};

using ClassMemberModifiers = FunctionModifiers;
