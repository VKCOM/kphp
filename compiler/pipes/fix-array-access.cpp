// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/fix-array-access.h"

#include "auto/compiler/vertex/vertex-types.h"
#include "common/algorithms/contains.h"
#include "compiler/data/class-data.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/inferring/public.h"
#include "compiler/operation.h"
#include <cassert>

static VertexPtr on_set(VertexAdaptor<op_set> set) {
  if (set->lhs()->type() != op_func_call) {
    return set;
  }

  auto func_call = set->lhs().try_as<op_func_call>();
  if (func_call->auto_inserted) {
    if (func_call->func_id->local_name() == "offsetGet") {
      auto obj_arg = func_call->args()[0];
      const auto *tpe = tinf::get_type(obj_arg); // funny

      assert(tpe);
      assert(tpe->get_real_ptype() == tp_Class);

      auto klass = tpe->class_type();

      const auto *method = klass->get_instance_method("offsetSet");

      if (!method) {
        kphp_error(method, fmt_format("Class {} does not implement offsetSet", klass->name).c_str());
        return set;
      }

      auto sub_val = set->rhs();

      auto key = func_call->args()[1];

      obj_arg.set_rl_type(val_r);
      auto zzz = VertexAdaptor<op_set_with_ret>::create(key, sub_val, obj_arg);
      zzz->set_method = method->function;

      return zzz;
    }
  }
  return set;
}

static VertexPtr on_unset(VertexAdaptor<op_unset> unset) {
  if (auto func_call = unset->expr().try_as<op_func_call>()) {
    if (func_call->func_id->name.find("offsetGet") != std::string::npos) {
      auto klass = func_call->func_id->class_id;
      // TODO assume here that all is good
      assert(klass && "bad klass for unset");

      const auto *unset_method = klass->get_instance_method("offsetUnset");
      assert(unset_method && "bad method for unset");

      func_call->str_val = unset_method->global_name();
      func_call->func_id = unset_method->function;
      return func_call;
    }
  }

  return unset;
}

static VertexPtr on_isset(VertexAdaptor<op_isset> isset) {
  if (auto func_call = isset->expr().try_as<op_func_call>()) {
    if (func_call->func_id->name.find("offsetGet") != std::string::npos) {
      auto klass = func_call->func_id->class_id;
      // TODO assume here that all is good
      assert(klass && "bad klass for isset");

      const auto *isset_method = klass->get_instance_method("offsetExists");
      assert(isset_method && "bad method for isset");

      func_call->str_val = isset_method->global_name();
      func_call->func_id = isset_method->function;
      return func_call;
    }
  }

  return isset;
}

static VertexPtr on_empty(VertexAdaptor<op_func_call> empty_call) {
  if (auto offset_get_call = empty_call->args()[0].try_as<op_func_call>()) {
    if (offset_get_call->func_id->name.find("offsetGet") != std::string::npos) {
      auto klass = offset_get_call->func_id->class_id;
      // TODO assume here that all is good
      assert(klass && "bad klass for isset");

      const auto *isset_method = klass->get_instance_method("offsetExists");
      assert(isset_method && "bad method for isset");

      auto exists_call = offset_get_call.clone();

      exists_call->str_val = isset_method->global_name();
      exists_call->func_id = isset_method->function;
      /*
      for empty smth like:
      op_log_or
        op_log_not
          op_func_call f$ClassName$exists
            ...
        op_func_call empty
          op_func_call f$ClassName$get
            ...
      */

      // TODO fix locations
      auto as_or = VertexAdaptor<op_log_or>::create(VertexAdaptor<op_log_not>::create(exists_call), empty_call);

      return as_or;
    }
  }

  return empty_call;
}

// TODO maybe on_enter? Think about it
// on_exit now generates x2 warnings, but may be the only correct way (may be not xD)
VertexPtr FixArrayAccessPass::on_exit_vertex(VertexPtr root) {
  if (auto set = root.try_as<op_set>()) {
    return on_set(set);
  }
  if (auto unset = root.try_as<op_unset>()) {
    return on_unset(unset);
  }
  if (auto isset = root.try_as<op_isset>()) {
    return on_isset(isset);
  }
  if (auto func_call = root.try_as<op_func_call>()) {
    if (func_call->func_id->is_extern() && func_call->func_id->name == "empty") {
      return on_empty(func_call);
    }
  }

  return root;
}
