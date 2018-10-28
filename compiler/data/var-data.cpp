#include "compiler/data/var-data.h"

#include "compiler/data/class-data.h"
#include "compiler/stage.h"

VarData::VarData(VarData::Type type_) :
  type_(type_),
  id(-1),
  param_i(),
  tinf_node(VarPtr(this)),
  init_val(nullptr),
  bad_vars(nullptr),
  is_constant(false),
  is_reference(false),
  uninited_flag(false),
  optimize_flag(false),
  tinf_flag(false),
  global_init_flag(false),
  needs_const_iterator_flag(false),
  marked_as_global(false),
  dependency_level(0) {}

void VarData::set_uninited_flag(bool f) {
  uninited_flag = f;
}

bool VarData::get_uninited_flag() {
  return uninited_flag;
}

string VarData::get_human_readable_name() const {
  return (this->class_id ? (this->class_id->name + " :: $" + this->name) : "$" + this->name);
}

const ClassMemberStaticField *VarData::as_class_static_field() const {
  kphp_assert(is_class_static_var() && class_id);
  return class_id->members.get_static_field(get_local_name_from_global_$$(name));
}

const ClassMemberInstanceField *VarData::as_class_instance_field() const {
  kphp_assert(is_class_instance_var() && class_id);
  return class_id->members.get_instance_field(name);
}

bool operator<(VarPtr a, VarPtr b) {
  int cmp_res = a->name.compare(b->name);

  if (cmp_res == 0) {
    {
      bool af{a->static_id};
      bool bf{b->static_id};
      if (af || bf) {
        if (af && bf) {
          return a->static_id < b->static_id;
        } else {
          return af < bf;
        }
      }
    }
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
