// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/fix-array-access.h"

#include "auto/compiler/vertex/vertex-types.h"
#include "compiler/compiler-core.h"
#include "common/algorithms/contains.h"
#include "compiler/data/class-data.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/inferring/public.h"

static VertexPtr on_unset(VertexAdaptor<op_unset> unset) {
  if (auto func_call = unset->expr().try_as<op_func_call>()) {
    if (!func_call->auto_inserted) {
      return unset;
    }
    if (func_call->func_id && vk::contains(func_call->func_id->name, "offsetGet")) {
      auto klass = func_call->func_id->class_id;
      kphp_assert_msg(klass, "Internal error: cannot get type of object for [] access");

      const auto *unset_method = klass->get_instance_method("offsetUnset");
      kphp_error(unset_method, fmt_format("Class {} does not implement \\ArrayAccess", klass->name).c_str());

      func_call->str_val = unset_method->global_name();
      func_call->func_id = unset_method->function;
      return func_call;
    }
  }

  return unset;
}

static VertexPtr on_isset(VertexAdaptor<op_isset> isset) {
  if (isset->was_eq3) {
    return isset;
  }

  // todo 
  // recursively search for op_fun_call offsetGet
  // and change on 
  // 1) offsetExists on higher level
  // 2) offsetExists + offsetGet on intermediate levels
  // but what to do with mixed...
  if (auto func_call = isset->expr().try_as<op_func_call>()) {
    if (!func_call->auto_inserted) {
      return isset;
    }
    if (vk::contains(func_call->func_id->name, "offsetGet")) {
      auto klass = func_call->func_id->class_id;
      kphp_assert_msg(klass, "Internal error: cannot get type of object for [] access");

      const auto *isset_method = klass->get_instance_method("offsetExists");
      kphp_error(isset_method, fmt_format("Class {} does not implement \\ArrayAccess", klass->name).c_str());

      func_call->str_val = isset_method->global_name();
      func_call->func_id = isset_method->function;
      return func_call;
    }
  }

  return isset;
}

static VertexPtr on_empty(VertexAdaptor<op_func_call> empty_call) {
  if (auto offset_get_call = empty_call->args()[0].try_as<op_func_call>()) {
    if (!offset_get_call->auto_inserted) {
      return empty_call;
    }
    if (offset_get_call->func_id && vk::contains(offset_get_call->func_id->name, "offsetGet")) {
      auto klass = offset_get_call->func_id->class_id;
      kphp_assert_msg(klass, "Internal error: cannot get type of object for [] access");

      const auto *isset_method = klass->get_instance_method("offsetExists");
      kphp_error(isset_method, fmt_format("Class {} does not implement \\ArrayAccess", klass->name).c_str());

      auto exists_call = offset_get_call.clone();
      exists_call->str_val = isset_method->global_name();
      exists_call->func_id = isset_method->function;

      /*
       * For empty($obj[42]) we have following tranfsormation:
       *
       * op_func_call empty
       *   op_func_call f$ClassName$offsetGet
       *     ...
       *
       *  --->
       *
       * op_log_or
       *   op_log_not
       *     op_func_call f$ClassName$offsetExists
       *       ...
       *   op_func_call empty // the original node
       *     op_func_call f$ClassName$offsetGet
       *       ...
       */

      auto as_or = VertexAdaptor<op_log_or>::create(VertexAdaptor<op_log_not>::create(exists_call), empty_call);
      as_or.set_location_recursively(empty_call);

      return as_or;
    }
  }

  return empty_call;
}

static VertexPtr on_log_not(VertexAdaptor<op_log_not> log_not_op) {
  auto inner = log_not_op->front();
  if (auto isset = inner.try_as<op_isset>(); isset && isset->was_eq3) {
    VertexPtr v = isset->front();
    bool has_index = v->type() == op_index;
    while (v->type() == op_index) {
      v = v.as<op_index>()->array();
    }

    if (has_index) {
      const auto *tpe = tinf::get_type(v);
      if (tpe->ptype() == tp_mixed || (tpe->ptype() == tp_Class && G->get_class("ArrayAccess")->is_parent_of(tpe->class_type()))) {
        return VertexAdaptor<op_eq3>::create(isset->front(), VertexAdaptor<op_null>::create());
      }
    }
  }
  return log_not_op;
}

VertexPtr FixArrayAccessPass::on_exit_vertex(VertexPtr root) {
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
  if (auto log_not = root.try_as<op_log_not>()) {
    return on_log_not(log_not);
  }

  return root;
}
