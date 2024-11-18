// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/fix-array-access.h"

#include "common/algorithms/contains.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-data.h"

namespace {

enum class CheckFunction { ISSET, EMPTY };

std::pair<VertexPtr, bool> fixup_check_function(VertexPtr cur, VertexPtr prev, FunctionPtr current_function, CheckFunction check_kind) {
  if (cur->type() == op_isset) {
    // aldready done
    return {cur, false};
  }

  bool resp = false;

  if (auto as_index = cur.try_as<op_index>()) {
    auto son_res = fixup_check_function(as_index->array(), cur, current_function, check_kind);
    resp |= son_res.second;
    as_index->array() = son_res.first;
    if (check_kind == CheckFunction::ISSET) {
      as_index->inside_isset = true;
    } else {
      as_index->inside_empty = true;
    }
  }
  if (auto as_func_call = cur.try_as<op_func_call>(); as_func_call && as_func_call->func_id->class_id && !as_func_call->args().empty()) {
    auto son_res = fixup_check_function(as_func_call->args()[0], cur, current_function, check_kind);
    resp |= son_res.second;
    as_func_call->args()[0] = son_res.first;
  }
  if (auto as_instance_prop = cur.try_as<op_instance_prop>(); as_instance_prop) {
    auto son_res = fixup_check_function(as_instance_prop->front(), cur, current_function, check_kind);
    resp |= son_res.second;
    as_instance_prop->front() = son_res.first;
  }

  if (auto func_call = cur.try_as<op_func_call>()) {
    if (!func_call->auto_inserted) {
      return {cur, false};
    }

    if (!vk::contains(func_call->func_id->name, "offsetGet")) {
      return {cur, false};
    }

    auto klass = func_call->func_id->class_id;
    kphp_assert_msg(klass, "Internal error: cannot get type of object for [] access");

    const auto *isset_method = klass->get_instance_method("offsetExists");
    kphp_error(isset_method, fmt_format("Class {} does not implement \\ArrayAccess", klass->name).c_str());

    if (prev->type() == op_index) {
      // The result of current index access is going to be read with another index access.
      // Codegen is corresponding macro
      auto offset = func_call->args()[1];
      auto response = VertexAdaptor<op_check_and_get>::create(offset, func_call->args()[0]);
      response->get_method = func_call->func_id;
      response->check_method = isset_method->function;
      response.set_location(cur);
      response->tinf_node.set_type(TypeData::get_type(PrimitiveType::tp_mixed));
      response->is_empty = check_kind == CheckFunction::EMPTY;

      return {response, false};
    }

    if (check_kind == CheckFunction::ISSET) {

      func_call->str_val = isset_method->global_name();
      func_call->func_id = isset_method->function;
      return {func_call, true};
    } else {

      auto exists_call = func_call.clone();
      exists_call->str_val = isset_method->global_name();
      exists_call->func_id = isset_method->function;

      auto as_or =
        VertexAdaptor<op_log_or>::create(VertexAdaptor<op_log_not>::create(exists_call), VertexAdaptor<op_eq2>::create(cur, VertexAdaptor<op_null>::create()));
      as_or.set_location_recursively(cur);

      return {as_or, true};
    }
  }

  return {cur, resp};
}

VertexPtr on_isset(VertexAdaptor<op_isset> isset, FunctionPtr current_function) {
  if (isset->was_eq3) {
    return isset;
  }

  auto res = fixup_check_function(isset->expr(), isset, current_function, CheckFunction::ISSET);
  if (res.second) {
    return res.first;
  }

  isset->expr() = res.first;
  return isset;
}

VertexPtr on_empty(VertexAdaptor<op_func_call> empty_call, FunctionPtr current_function) {
  auto res = fixup_check_function(empty_call->args()[0], empty_call, current_function, CheckFunction::EMPTY);
  if (res.second) {
    return res.first;
  }

  empty_call->args()[0] = res.first;
  return empty_call;
}

VertexPtr on_log_not(VertexAdaptor<op_log_not> log_not_op) {
  // In some previous passes we had an optimization: `$x === null` --> `isset($x)`
  // but it's semantically incorrect for objects implementing ArrayAccess
  // and we have explicit `===` comparison.
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

VertexPtr on_unset(VertexAdaptor<op_unset> unset) {
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

} // namespace

VertexPtr FixArrayAccessPass::on_exit_vertex(VertexPtr root) {
  if (auto isset = root.try_as<op_isset>()) {
    return on_isset(isset, current_function);
  }
  if (auto func_call = root.try_as<op_func_call>()) {
    if (func_call->func_id->is_extern() && func_call->func_id->name == "empty") {
      return on_empty(func_call, current_function);
    }
  }
  if (auto log_not = root.try_as<op_log_not>()) {
    return on_log_not(log_not);
  }
  if (auto unset = root.try_as<op_unset>()) {
    return on_unset(unset);
  }

  return root;
}
