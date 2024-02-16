// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/var-data.h"

#include "compiler/data/class-data.h"
#include "compiler/stage.h"

VarData::VarData(VarData::Type type_) :
  type_(type_) {}

void VarData::set_uninited_flag(bool f) {
  uninited_flag = f;
}

bool VarData::get_uninited_flag() {
  return uninited_flag;
}

std::string VarData::as_human_readable() const {
  return (this->class_id ? (this->class_id->as_human_readable() + "::$" + get_local_name_from_global_$$(this->name)) : "$" + this->name);
}

const ClassMemberStaticField *VarData::as_class_static_field() const {
  kphp_assert(is_class_static_var() && class_id);
  return class_id->members.get_static_field(get_local_name_from_global_$$(name));
}

const ClassMemberInstanceField *VarData::as_class_instance_field() const {
  kphp_assert(is_class_instance_var() && class_id);
  return class_id->members.get_instance_field(name);
}

// TODO Dirty HACK, should be removed !!
bool VarData::does_name_eq_any_builtin_global(const std::string &name) {
  static const std::unordered_set<std::string> names = {
    "_SERVER", "_GET", "_POST", "_FILES", "_COOKIE", "_REQUEST", "_ENV", "argc", "argv",
    "d$PHP_SAPI"};
  return names.find(name) != names.end();
}

// todo del
bool operator<(VarPtr a, VarPtr b) {
  int cmp_res = a->name.compare(b->name);

  if (cmp_res == 0) {
    {
      bool af{a->holder_func};
      bool bf{b->holder_func};
      if (af || bf) {
        if (af && bf) {
          return a->holder_func < b->holder_func;
        } else {
          return af < bf;
        }
      }
    }
    return false;
  }

  if (a->dependency_level != b->dependency_level) {
    return a->dependency_level < b->dependency_level;
  }

  return cmp_res < 0;
}
